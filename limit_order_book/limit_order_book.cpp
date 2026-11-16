#include "limit_order_book.h"
#include "matching_engine.h"

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

                // checking if there is no price collision (e. g. prices of 100 and 356 will collide)
                const MEOrdersAtPrice *existingLevel = ordersAtPriceHashMap[priceToIndex(priceVal)];
                
                if(UNLIKELY(qty == 0 || qty == QUANTITY_INVALID || priceVal == PRICE_INVALID || 
                    (sideVal != side::BUY && sideVal != side::SELL) || (tickerId != _bookTickerId)
                    || (existingLevel && (existingLevel -> _priceVal != priceVal)))) {

                    // reject the client's order
                    _clientResponse = clientResponseTypeDef(clientResponseTypeEnum::REJECTED, 
                        clientId, tickerId, ORDER_ID_INVALID, clientOrderId, sideVal, priceVal, 
                        qty, QUANTITY_INVALID);

                    _matchingEngine -> sendClientResponse(_clientResponse);

                    return;

                }

                // generate 
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

                    // checking if we have successfully allocated the order (there is enough space in the Memory Pool)
                    if(UNLIKELY(order == nullptr)) {

                        // std::cerr << "MEORderBook::add() failed: memory pool is out of space" << std::endl;
                        _logger -> log("MEORderBook::add() failed: memory pool of orders is out of space");

                        // the quantity of 0 was executed
                        _clientResponse = clientResponseTypeDef{clientResponseTypeEnum::CANCELED, clientId, 
                            tickerId, newOrderId, clientOrderId, sideVal, priceVal, 0, leavesQuantity};

                        _matchingEngine -> sendClientResponse(_clientResponse);

                        return;

                    }

                    if (addOrder(order) == false) {

                        _ordersPool.deallocate(order);
                        return;

                    }

                    _marketUpdate = marketDataTypeDef{MarketUpdateType::ADD, newOrderId, tickerId, priceVal, leavesQuantity, 
                    prio, sideVal};

                    _matchingEngine -> sendMarketUpdate(_marketUpdate);

                }

            }

    
    /**
    * @brief 
    */
    void MEOrderBook::cancel(client_id clientId, order_id orderId, ticker_id tickerId) noexcept {

        // getting a pointer to the order from the hashmap
        MEOrder *order = ordersHashMap.at(orderId % ME_MAX_ORDER_IDS);

        const bool isCancelable = (order) && (order -> _marketOrderID == orderId) && 
            (tickerId == _bookTickerId) && (order -> _clientID == clientId);

        // preparing request and multicast update and removing the order
        if(UNLIKELY(!isCancelable)) {

            _clientResponse = clientResponseTypeDef{clientResponseTypeEnum::CANCEL_REJECTED, clientId, 
                tickerId, orderId, ORDER_ID_INVALID, side::INVALID, PRICE_INVALID, QUANTITY_INVALID, 
                QUANTITY_INVALID};

        }

        else {

            _clientResponse = clientResponseTypeDef{clientResponseTypeEnum::CANCELED, clientId, 
                tickerId, orderId, order -> _clientOrderID, order -> _meOrderSide, 
                order -> _price, QUANTITY_INVALID, order -> _qty};

            _marketUpdate = marketDataTypeDef{MarketUpdateType::CANCEL, orderId, tickerId, 
                order -> _price, order -> _qty, order -> _prio, order -> _meOrderSide};

            removeOrder(order);

            _matchingEngine -> sendMarketUpdate(_marketUpdate);

        }

        _matchingEngine -> sendClientResponse(_clientResponse);

    }

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
        _clientResponse = clientResponseTypeDef(clientResponseTypeEnum::FILLED, clientId, 
                            tickerId, newMarketOrderId, clientOrderId, sideVal, order -> _price, fillQty, *leavesQty);
        _matchingEngine -> sendClientResponse(_clientResponse);

        // reply to the owner of the passive order
        _clientResponse = clientResponseTypeDef(clientResponseTypeEnum::FILLED, order -> _clientID, 
                            tickerId, order -> _marketOrderID, order -> _clientOrderID, order -> _meOrderSide, 
                            matchingOrder -> _price, fillQty, order -> _qty);
        _matchingEngine -> sendClientResponse(_clientResponse);

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

    /**
     * @brief prints current constents of the book (vibecoded)
     */
    auto MEOrderBook::print() noexcept -> void {

        for(auto [head, name] : {std::pair{_asksByPrice, "ASKS"}, std::pair{_bidsByPrice, "BIDS"}}) {

            std::cout << "---- " << name << " ----\n";
            if(!head) continue;

            auto level = head;
            do {

                std::cout << level -> _priceVal << " |";
                auto order = level -> firstMeOrder;

                do {
                    std::cout << ' ' << order -> _qty << "(#" << order -> _marketOrderID << ')';
                    order = order -> _nextOrder;
                } while(order != level -> firstMeOrder);

                std::cout << '\n';
                level = level -> _nextEntry;

            } while(level != head);
        }
    }

    /**
    * @brief adds order to the list of orders
    *        at the certain price level
    * @param &order is the refernce to the order instance we want to insert 
    *        to the book
    */
    bool MEOrderBook::addOrder(MEOrder *order) {

        // getting orders at price at the desired 
        // price level
        const auto ordersAtPrice = getOrdersAtPrice(order -> _price);

        // the order is the first at its price level 
        if(!ordersAtPrice) {

            order -> _nextOrder = order -> _prevOrder = order;

            // allocating new slot in the pool 
            MEOrdersAtPrice *newOrdersAtPrice = _ordersAtPricePool.allocate(order -> _meOrderSide, 
                                                                           order -> _price, 
                                                                           nullptr, 
                                                                           nullptr);

            if(newOrdersAtPrice == nullptr) {

                _logger -> log("MEOrderBook::addOrder: got nullptr after allocate()");
            
                _clientResponse = clientResponseTypeDef(clientResponseTypeEnum::CANCELED, order -> _clientID, 
                    order -> _tickerID, ORDER_ID_INVALID, order ->_clientOrderID, order -> _meOrderSide, 
                    order -> _price, order -> _qty, PRICE_INVALID);

                _matchingEngine -> sendClientResponse(_clientResponse);

                return false;

            }

            newOrdersAtPrice -> firstMeOrder = order;

            // register order at the hashmap
            ordersHashMap.at(order -> _marketOrderID % ME_MAX_ORDER_IDS) = order;


            addOrdersAtPrice(newOrdersAtPrice);

            return true;

        }

        // the order price level is already present in the order book
        else {

            auto firstOrder = ordersAtPrice -> firstMeOrder;

            firstOrder -> _prevOrder -> _nextOrder = order;

            order -> _prevOrder = firstOrder -> _prevOrder;

            order -> _nextOrder = firstOrder;

            firstOrder -> _prevOrder = order;

            // register order at the hashmap
            ordersHashMap.at(order -> _marketOrderID % ME_MAX_ORDER_IDS) = order;

            return true;


        }

    }

}