#include <atomic>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "limit_order_book.h"
#include "matching_engine.h"

namespace {

using namespace exchange;

constexpr ticker_id TEST_TICKER = 1;

std::string makeLogFileName() {
    static std::atomic<unsigned> counter{0};
    return "/tmp/limit_order_book_test_" +
           std::to_string(counter.fetch_add(1)) +
           ".log";
}

template <typename Queue>
auto drainQueue(Queue& queue) {

    // removing const and volatile 
    using Value = std::remove_cv_t<
        // removing references 
        std::remove_reference_t<decltype(*queue.getNextSlotToRead())>>;

    std::vector<Value> result;

    while (auto* value = queue.getNextSlotToRead()) {
        result.push_back(*value);
        queue.updateReadIndex();
    }

    return result;
}

class LimitOrderBookTest : public ::testing::Test {
protected:
    ClientReqLFQueue requestQueue{128};
    CLientResponseLFQueue responseQueue{128};
    MarketUpdateLFQueue marketUpdateQueue{128};

    Logger logger{makeLogFileName()};
    MatchingEngine matchingEngine{
        &marketUpdateQueue,
        &requestQueue,
        &responseQueue,
        &logger
    };
    MEOrderBook book{TEST_TICKER, &matchingEngine, &logger};

    void addOrder(
        client_id clientId,
        order_id clientOrderId,
        side orderSide,
        price orderPrice,
        quantity orderQty) {
        book.add(clientId, clientOrderId, TEST_TICKER,
                 orderSide, orderPrice, orderQty);
    }

    auto responses() {
        return drainQueue(responseQueue);
    }

    auto updates() {
        return drainQueue(marketUpdateQueue);
    }
};

TEST_F(LimitOrderBookTest, AddsRestingOrderAndPublishesAdd) {
    addOrder(10, 1001, side::SELL, 105, 7);

    const auto response = responses();
    const auto update = updates();

    ASSERT_EQ(response.size(), 1);
    EXPECT_EQ(response[0].responseStatus, clientResponseTypeEnum::ACCEPTED);
    EXPECT_EQ(response[0].clientId, 10);
    EXPECT_EQ(response[0].clientOrderId, 1001);
    EXPECT_EQ(response[0].leavesQty, 7);

    ASSERT_EQ(update.size(), 1);
    EXPECT_EQ(update[0].marketUpdateType, MarketUpdateType::ADD);
    EXPECT_EQ(update[0].priceValue, 105);
    EXPECT_EQ(update[0].quant, 7);
    EXPECT_EQ(update[0].sideValue, side::SELL);
}

TEST_F(LimitOrderBookTest, FullyMatchesAndRemovesRestingOrder) {
    addOrder(10, 1001, side::SELL, 105, 5);
    responses();
    updates();

    addOrder(20, 2001, side::BUY, 105, 5);

    const auto response = responses();
    const auto update = updates();

    ASSERT_EQ(response.size(), 3);
    EXPECT_EQ(response[0].responseStatus, clientResponseTypeEnum::ACCEPTED);
    EXPECT_EQ(response[1].responseStatus, clientResponseTypeEnum::FILLED);
    EXPECT_EQ(response[1].clientId, 20);
    EXPECT_EQ(response[1].qty, 5);
    EXPECT_EQ(response[2].responseStatus, clientResponseTypeEnum::FILLED);
    EXPECT_EQ(response[2].clientId, 10);
    EXPECT_EQ(response[2].qty, 5);
    EXPECT_EQ(response[2].leavesQty, 0);

    ASSERT_EQ(update.size(), 2);
    EXPECT_EQ(update[0].marketUpdateType, MarketUpdateType::TRADE);
    EXPECT_EQ(update[0].priceValue, 105);
    EXPECT_EQ(update[0].quant, 5);
    EXPECT_EQ(update[1].marketUpdateType, MarketUpdateType::CANCEL);

    // Проверяем, что полностью исполненный ордер больше не стоит в книге.
    addOrder(30, 3001, side::BUY, 105, 1);

    const auto followUpResponse = responses();
    const auto followUpUpdate = updates();

    ASSERT_EQ(followUpResponse.size(), 1);
    EXPECT_EQ(followUpResponse[0].responseStatus,
              clientResponseTypeEnum::ACCEPTED);
    ASSERT_EQ(followUpUpdate.size(), 1);
    EXPECT_EQ(followUpUpdate[0].marketUpdateType, MarketUpdateType::ADD);
}

TEST_F(LimitOrderBookTest, PartialFillUpdatesPassiveOrderLeaves) {
    addOrder(10, 1001, side::SELL, 105, 10);
    responses();
    updates();

    addOrder(20, 2001, side::BUY, 105, 4);

    const auto response = responses();
    const auto update = updates();

    ASSERT_EQ(response.size(), 3);
    EXPECT_EQ(response[1].responseStatus, clientResponseTypeEnum::FILLED);
    EXPECT_EQ(response[1].clientId, 20);
    EXPECT_EQ(response[1].qty, 4);
    EXPECT_EQ(response[1].leavesQty, 0);

    EXPECT_EQ(response[2].responseStatus, clientResponseTypeEnum::FILLED);
    EXPECT_EQ(response[2].clientId, 10);
    EXPECT_EQ(response[2].qty, 4);
    EXPECT_EQ(response[2].leavesQty, 6);

    ASSERT_EQ(update.size(), 2);
    EXPECT_EQ(update[0].marketUpdateType, MarketUpdateType::TRADE);
    EXPECT_EQ(update[0].quant, 4);
    EXPECT_EQ(update[1].marketUpdateType, MarketUpdateType::MODIFY);
    EXPECT_EQ(update[1].quant, 6);
}

TEST_F(LimitOrderBookTest, IncomingRemainderRestsOnBook) {
    addOrder(10, 1001, side::SELL, 105, 4);
    responses();
    updates();

    addOrder(20, 2001, side::BUY, 105, 10);

    const auto response = responses();
    const auto update = updates();

    ASSERT_EQ(response.size(), 3);
    EXPECT_EQ(response[1].responseStatus, clientResponseTypeEnum::FILLED);
    EXPECT_EQ(response[1].clientId, 20);
    EXPECT_EQ(response[1].qty, 4);
    EXPECT_EQ(response[1].leavesQty, 6);

    ASSERT_EQ(update.size(), 3);
    EXPECT_EQ(update[0].marketUpdateType, MarketUpdateType::TRADE);
    EXPECT_EQ(update[1].marketUpdateType, MarketUpdateType::CANCEL);
    EXPECT_EQ(update[2].marketUpdateType, MarketUpdateType::ADD);
    EXPECT_EQ(update[2].sideValue, side::BUY);
    EXPECT_EQ(update[2].quant, 6);
}

TEST_F(LimitOrderBookTest, MatchesBestPriceFirst) {
    addOrder(10, 1001, side::SELL, 106, 2);
    responses();
    updates();

    addOrder(11, 1002, side::SELL, 105, 3);
    responses();
    updates();

    addOrder(20, 2001, side::BUY, 106, 5);

    const auto response = responses();
    const auto update = updates();

    ASSERT_EQ(response.size(), 5);
    ASSERT_EQ(update.size(), 4);

    EXPECT_EQ(update[0].marketUpdateType, MarketUpdateType::TRADE);
    EXPECT_EQ(update[0].priceValue, 105);
    EXPECT_EQ(update[0].quant, 3);

    EXPECT_EQ(update[2].marketUpdateType, MarketUpdateType::TRADE);
    EXPECT_EQ(update[2].priceValue, 106);
    EXPECT_EQ(update[2].quant, 2);
}

TEST_F(LimitOrderBookTest, PreservesFifoAtSamePrice) {
    addOrder(10, 1001, side::SELL, 105, 5);
    const auto firstAccepted = responses();
    updates();

    addOrder(11, 1002, side::SELL, 105, 5);
    const auto secondAccepted = responses();
    updates();

    ASSERT_EQ(firstAccepted.size(), 1);
    ASSERT_EQ(secondAccepted.size(), 1);

    const order_id firstMarketOrderId = firstAccepted[0].marketOrderId;
    const order_id secondMarketOrderId = secondAccepted[0].marketOrderId;

    addOrder(20, 2001, side::BUY, 105, 7);

    const auto response = responses();

    ASSERT_EQ(response.size(), 5);
    EXPECT_EQ(response[0].responseStatus, clientResponseTypeEnum::ACCEPTED);

    // Первая пассивная заявка исполняется целиком, затем вторая частично.
    EXPECT_EQ(response[2].clientId, 10);
    EXPECT_EQ(response[2].marketOrderId, firstMarketOrderId);
    EXPECT_EQ(response[2].qty, 5);

    EXPECT_EQ(response[4].clientId, 11);
    EXPECT_EQ(response[4].marketOrderId, secondMarketOrderId);
    EXPECT_EQ(response[4].qty, 2);
    EXPECT_EQ(response[4].leavesQty, 3);
}

TEST_F(LimitOrderBookTest, CancelRemovesOrderAndRejectsSecondCancel) {
    addOrder(10, 1001, side::SELL, 105, 5);
    const auto accepted = responses();
    const auto addUpdate = updates();

    ASSERT_EQ(accepted.size(), 1);
    ASSERT_EQ(addUpdate.size(), 1);

    const order_id marketOrderId = accepted[0].marketOrderId;

    book.cancel(10, marketOrderId, TEST_TICKER);

    const auto cancelResponse = responses();
    const auto cancelUpdate = updates();

    ASSERT_EQ(cancelResponse.size(), 1);
    EXPECT_EQ(cancelResponse[0].responseStatus,
              clientResponseTypeEnum::CANCELED);
    EXPECT_EQ(cancelResponse[0].marketOrderId, marketOrderId);

    ASSERT_EQ(cancelUpdate.size(), 1);
    EXPECT_EQ(cancelUpdate[0].marketUpdateType, MarketUpdateType::CANCEL);

    // Повторная отмена должна быть отклонена.
    book.cancel(10, marketOrderId, TEST_TICKER);

    const auto repeatedCancelResponse = responses();
    const auto repeatedCancelUpdate = updates();

    ASSERT_EQ(repeatedCancelResponse.size(), 1);
    EXPECT_EQ(repeatedCancelResponse[0].responseStatus,
              clientResponseTypeEnum::CANCEL_REJECTED);
    EXPECT_TRUE(repeatedCancelUpdate.empty());
}

TEST_F(LimitOrderBookTest, RejectsCancelFromDifferentClient) {
    addOrder(10, 1001, side::SELL, 105, 5);
    const auto accepted = responses();
    updates();

    ASSERT_EQ(accepted.size(), 1);

    book.cancel(99, accepted[0].marketOrderId, TEST_TICKER);

    const auto response = responses();
    const auto update = updates();

    ASSERT_EQ(response.size(), 1);
    EXPECT_EQ(response[0].responseStatus,
              clientResponseTypeEnum::CANCEL_REJECTED);
    EXPECT_TRUE(update.empty());

    // Ордер остался в книге: другой клиентский BUY его исполняет.
    addOrder(20, 2001, side::BUY, 105, 5);

    const auto fillResponse = responses();
    const auto fillUpdate = updates();

    ASSERT_EQ(fillResponse.size(), 3);
    EXPECT_EQ(fillResponse[1].responseStatus, clientResponseTypeEnum::FILLED);
    EXPECT_EQ(fillResponse[2].clientId, 10);
    ASSERT_EQ(fillUpdate.size(), 2);
    EXPECT_EQ(fillUpdate[0].marketUpdateType, MarketUpdateType::TRADE);
}

} // namespace