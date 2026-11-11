#include "limit_order_book.h"

constexpr std::size_t MEMPOOL_BOOK_SIZE = 512;

namespace exchange {

    MEOrderBook::MEOrderBook(ticker_id tickerId, MatchingEngine *matchingEngine, Logger *logger) : 
     _matchingEngine(matchingEngine), _bookTickerId(tickerId), _logger(logger), 
     _ordersAtPricePool(MEMPOOL_BOOK_SIZE), _ordersPool(MEMPOOL_BOOK_SIZE) { }

    MEOrderBook::~MEOrderBook() { }

    /**
     * @brief checks if the order matches some of the orders in the book. If it doesn't
     *        adds the order to the order book 
     * @return none
     */
    auto MEOrderBook::add(client_id clientId, order_id clientOrderId, ticker_id tickerId, 
            side sideVal, price priceVal, quantity qty) noexcept -> void {

                const auto newOrderId = generateNewMarketOrderId();

                // constructing response to client after "add" request
                _clientResponse = clientResponseTypeDef {clientResponseTypeEnum::ACCEPTED, clientId,  
                tickerId, newOrderId, clientOrderId, sideVal, priceVal, 0, qty};

                _matchingEngine -> sendClientResponse(_clientResponse);

                // checks whether a partial or complete match can be made.
                // This variable stores the remaining quantity of this order after
                // the matching event. For orders that fully executre leavesQuantity = 0.
                quantity leavesQuantity = checkForMatch(clientId, clientOrderId, tickerId, 
                    sideVal, priceVal, qty, newOrderId);

                // if the order was not executed completely, add it to the book
                if (LIKELY(leavesQuantity)) {

                    const auto prio = getNextPriority(priceVal);

                    // allocating new slot in the memory pool for 
                    // this order
                    auto order = _ordersPool.allocate(tickerId, clientId, clientOrderId, newOrderId, 
                        sideVal, prio, priceVal, leavesQuantity);

                    addOrder(order);

                    _marketUpdate = marketDataTypeDef{MarketUpdateType::ADD, newOrderId, tickerId, priceVal, leavesQuantity, 
                    prio, sideVal};

                    _matchingEngine -> sendMarketUpdate(_marketUpdate);

                }

            }

    
    /**
    * @brief 
    * @
    */
   /*
    auto MEOrderBook::cancel(client_id clientId, order_id orderId, ticker_id tickerId) noexcept -> void {

        // constructing response to the client
        _clientResponse = clientResponseTypeDef{clientResponseTypeEnum::CANCELED}


    }
   */

  /**
    * @brief executes a single deal between an incoming order and the order that is already in the book (passive order)
    * @param tickerId
    * @param clientId is the client that sent the aggressive order
    * @param sideVal buy/sell
    * @param clientOrderId id of the order on the client-side
    * @param newMarketOrderId id of the order within the trading system (that will be unique
    *                         exactly in the trading system)
    * @param *matchingOrder is the pointer to the order that we match with. It is a head
    *        of the FIFO sequence of a certain price level (passive order)
    * @param *leavesQty how many units left on the aggressive side after the match 
    * 
    */
   void MEOrderBook::match(ticker_id tickerId, client_id clientId, side sideVal, order_id clientOrderId, 
            order_id newMarketOrderId, MEOrder *matchingOrder, quantity *leavesQty) noexcept {

        MEOrder *order     = matchingOrder;
        const quantity orderQty  = order -> _qty;
        const quantity fillQty = std::min(orderQty, *leavesQty);

        // reducing the quantity of passive and aggressive 
        // order by the size of the smallest order. So 
        // at least on the order executes fully.
        *leavesQty    -= fillQty;
        order -> _qty -= fillQty;

        // reply to the client (aggressor)
        _meClientResponse = clientResponseTypeDef(clientResponseTypeEnum::FILLED, clientId, 
                            tickerId, newMarketOrderId, clientOrderId, sideVal, order -> _price, fillQty, *leavesQty);
        _matchingEngine -> sendClientResponse(_meClientResponse);

        // reply to the owner of the passive order
        _meClientResponse = clientResponseTypeDef(clientResponseTypeEnum::FILLED, order -> _clientID, 
                            tickerId, order -> _marketOrderID, order -> _clientOrderID, order -> _meOrderSide, 
                            matchingOrder -> _price, fillQty, order -> _qty);
        _matchingEngine -> sendClientResponse(_meClientResponse);

        // sending multicast market update

        // we do not disclose the information of order ids so the order id is invalid 
        // the deal is executed with the price of passive order so the price is set as
        // matchingOrder -> _price
        _marketUpdate = marketDataTypeDef(MarketUpdateType::TRADE, ORDER_ID_INVALID, tickerId, matchingOrder -> _price, 
            fillQty, PRIORITY_INVALID, sideVal);

        _matchingEngine -> sendMarketUpdate(_marketUpdate);

        // if the order was executed completely
        if(order -> _qty == 0) {

            // we cancelling the order 
            _marketUpdate = marketDataTypeDef(MarketUpdateType::CANCEL, order -> _marketOrderID, tickerId, 
            order -> _price, orderQty, PRIORITY_INVALID, order -> _meOrderSide);

            _matchingEngine -> sendMarketUpdate(_marketUpdate);

            // removing the passive order from the order book
            removeOrder(order);

        }

        // order was not executed completely
        else {

            _marketUpdate = marketDataTypeDef(MarketUpdateType::MODIFY, order -> _marketOrderID, tickerId, 
            order -> _price, order -> _qty, order -> _prio, order -> _meOrderSide);

            _matchingEngine -> sendMarketUpdate(_marketUpdate);

        }

    }

    /**
     * @brief executes all possible deals between a passive order and aggressive order
     */
    quantity MEOrderBook::checkForMatch(client_id clientId, order_id orderId, ticker_id tickerId, 
            side sideVal, price priceVal, quantity q, order_id newOrderId) noexcept {

                quantity leavesQty = q;

                if(sideVal == side::BUY) {

                    // executing all possible deals
                    while(leavesQty && _asksByPrice) {

                        MEOrder* ask = _asksByPrice -> firstMeOrder;

                        // if the suggested purchase price is less than the 
                        // minimal price of the ask we should break the cycle 
                        if(LIKELY(priceVal < ask -> _price))
                            break;

                        match(tickerId, clientId, sideVal, orderId, newOrderId, ask, &leavesQty);

                    }

                }

                if (sideVal == side::SELL) {

                    while(leavesQty && _bidsByPrice) {

                        MEOrder *bid = _bidsByPrice -> firstMeOrder;

                        // if the price we want to sell by is bigger than purchase price 
                        // in any passive order we should break the cycle. The 
                        if(LIKELY(priceVal > bid -> _price))
                            break;

                        match(tickerId, clientId, sideVal, orderId, newOrderId, bid, &leavesQty);

                    }

                }

                return leavesQty;
            }

}