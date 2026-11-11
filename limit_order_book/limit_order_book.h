#pragma once

#include <algorithm>

#include "md_protocol.h"
#include "matching_engine.h"
#include "logger.h"
#include "mem_pool.h"
#include "me_order_struct.h"

namespace exchange {

    class MEOrderBook {
    public:

        explicit MEOrderBook(ticker_id tickerId, MatchingEngine *matchingEngine, Logger *logger);

        ~MEOrderBook();

        // removing move and copy opeartors
        MEOrderBook(const MEOrderBook&)              = delete;
        MEOrderBook &operator = (const MEOrderBook&) = delete;
        MEOrderBook(MEOrderBook &&)                  = delete;
        MEOrderBook &operator = (MEOrderBook &&)     = delete; 

        // function that processes new order after client's request.
        // If the order fully matches with another order, these orders are removed.
        // Otherwise the remaining quantity is pushed to the order book
        auto add(client_id clientId, order_id clientOrderId, 
            ticker_id tickerId, side sideVal, 
            price priceVal, quantity qty) noexcept -> void; 

        // function
        auto cancel(client_id clientId, order_id orderId, ticker_id tickerID) noexcept -> void;

        // funtion that prints the current 
        // contents of the limit order book
        auto print() noexcept -> void;


        // the response that is sent to the client
        clientResponseTypeDef _meClientResponse;

    private:

        Logger *_logger;

        // the ticker ID of this order book
        ticker_id _bookTickerId = TICKER_ID_INVALID;

        MatchingEngine *_matchingEngine = nullptr;


        // Memory Pool where MEOrdersAtPrice are stored
        // we have two separate sequences for bids and for asks
        MemoryPool<MEOrdersAtPrice> _ordersAtPricePool;

        // Memory where all the orders are stored
        MemoryPool<MEOrder> _ordersPool;

        // two separate books for bids and asks
        MEOrdersAtPrice *_bidsByPrice = nullptr;
        MEOrdersAtPrice *_asksByPrice = nullptr;

        // hashmaps that store orders and price levels
        OrderHashMap         ordersHashMap{};
        OrdersAtPriceHashMap ordersAtPriceHashMap{};


        order_id _nextMarketOrderId = 1;

        // keeps data for responding to client's requests
        clientResponseTypeDef _clientResponse;

        // keeps data for multicast sending of the updates in order book
        marketDataTypeDef _marketUpdate;

        /**
         * @brief returns the _nextMarketOrderId and increments it 
         *        the next time the method is called
         * @param none
         * @return _nextMarketOrderId++
         */
        auto generateNewMarketOrderId() noexcept -> order_id {

            return _nextMarketOrderId++;

        }

        /**
         * @brief converts price argument into index that ranges between 
         *        0 and ME_MAX_PRICE_LEVELS
         * @param none
         * @return index
         */
        auto priceToIndex(price priceArg) const noexcept {

            return (priceArg % ME_MAX_PRICE_LEVELS);

        } 

        /**
         * @brief adds new price level to the MEOrdersAtPrice. 
         *        It should be called if the new order price level
         *        is not present in the order book. It inserts the order 
         *        to the MEOrdersAtPrice to keep it sorted (bids are sorted from highest to lowest,
         *        asks are sorted from lowest to highest) 
         */
        void addOrdersAtPrice(MEOrdersAtPrice *newOrderAtPrice) noexcept {

            // register this level in the hash table of price-level 
            ordersAtPriceHashMap.at(priceToIndex(newOrderAtPrice -> _priceVal)) = newOrderAtPrice;

            // choosing what do we wrok with: bids or asks 
            const auto bestOrdersByPrice = (newOrderAtPrice -> _sideVal == side::BUY ? _bidsByPrice : 
                _asksByPrice);

            // if the current side is empty, the newOrderAtPrice becomes first
            if(UNLIKELY(!bestOrdersByPrice)) {

                (newOrderAtPrice -> _sideVal == side::BUY ? _bidsByPrice : _asksByPrice) = newOrderAtPrice;

                // only one element in the ring
                newOrderAtPrice -> _nextEntry = newOrderAtPrice -> _prevEntry = newOrderAtPrice;

            }

            else {

                // target is head of the bidsByPrice or asksByPrice
                auto target = bestOrdersByPrice;

                // if the new level is worse than target
                // what is actually "worse"?
                bool addAfter = 
                (newOrderAtPrice ->_sideVal == side::SELL && newOrderAtPrice ->_priceVal > target -> _priceVal) || 
                (newOrderAtPrice -> _sideVal == side::BUY && newOrderAtPrice -> _priceVal < target -> _priceVal);

                if(addAfter) {

                    target = target -> _nextEntry;

                    addAfter = 
                    (newOrderAtPrice ->_sideVal == side::SELL && newOrderAtPrice ->_priceVal > target -> _priceVal) || 
                    (newOrderAtPrice -> _sideVal == side::BUY && newOrderAtPrice -> _priceVal < target -> _priceVal);

                }

                while(addAfter && target != bestOrdersByPrice) {

                    addAfter = 
                    (newOrderAtPrice ->_sideVal == side::SELL && newOrderAtPrice ->_priceVal > target -> _priceVal) || 
                    (newOrderAtPrice -> _sideVal == side::BUY && newOrderAtPrice -> _priceVal < target -> _priceVal);

                    if(addAfter)
                        target = target -> _nextEntry;


                }

                // in case we are adding order after the target
                if(addAfter) {

                    if(target == bestOrdersByPrice) 
                        target = bestOrdersByPrice -> _prevEntry;

                    newOrderAtPrice -> _prevEntry = target;
                    target -> _nextEntry -> _prevEntry = newOrderAtPrice;
                    newOrderAtPrice -> _nextEntry = target -> _nextEntry;
                    target -> _nextEntry = newOrderAtPrice; 

                }

                // in case we are adding order before the target
                else {

                    newOrderAtPrice -> _prevEntry = target -> _prevEntry;
                    newOrderAtPrice -> _nextEntry = target;
                    target -> _prevEntry -> _nextEntry = newOrderAtPrice;
                    target -> _prevEntry = newOrderAtPrice;

                    if((newOrderAtPrice ->_sideVal == side::SELL && newOrderAtPrice ->_priceVal < bestOrdersByPrice -> _priceVal) || 
                    (newOrderAtPrice -> _sideVal == side::BUY && newOrderAtPrice -> _priceVal > bestOrdersByPrice -> _priceVal)) {

                        // target -> _nextEntry = (target -> _nextEntry == bestOrdersByPrice ? newOrderAtPrice : target ->_nextEntry);
                        (newOrderAtPrice -> _sideVal == side::BUY ? _bidsByPrice : _asksByPrice) = newOrderAtPrice;

                    }

                }

            }

        }

        /**
         * @brief removes all orders at the level of priceVal and
         *        the side of sideVal from the order book
         */
        void removeOrdersAtPrice(side sideVal, price priceVal) {

            // choosing between buy orders and sell orders
            const auto bestOrdersByPrice = (sideVal == side::BUY ? _bidsByPrice : _asksByPrice);

            auto ordersAtPrice = getOrdersAtPrice(priceVal);

            // checking if the current state (buy or sell)
            // consists only of one price (so the ring is only made)
            // of one element
            if(UNLIKELY(ordersAtPrice -> _nextEntry == ordersAtPrice)) {

                // after deleting the only level the head points to nullptr
                (sideVal == side::BUY ? _bidsByPrice : _asksByPrice) = nullptr;

            }

            else {

                // performing a deletion of the price level from the linked list
                ordersAtPrice -> _prevEntry -> _nextEntry = ordersAtPrice -> _nextEntry;
                ordersAtPrice -> _nextEntry -> _prevEntry = ordersAtPrice -> _prevEntry;


                // if the best level was deleted we should set the next one
                // as a head of the current side
                if(ordersAtPrice == bestOrdersByPrice) {

                    (sideVal == side::BUY ? _bidsByPrice : _asksByPrice) = ordersAtPrice -> _nextEntry;

                }

                // deleting the connections of the level we are deleting
                ordersAtPrice -> _nextEntry = ordersAtPrice -> _prevEntry = nullptr;

            }

            // making the entry in the hashtable empty
            ordersAtPriceHashMap.at(priceToIndex(priceVal)) = nullptr;

            // deallocating the price level from the pool
            _ordersAtPricePool.deallocate(ordersAtPrice);



        }

        /**
         * @brief returns reference to the first order at the demanded price
         * @param priceLevel is the desired price level
         * @return reference to the MEOrdersAtPrice member
         *         which contains reference to the order list of demanded price
         */
        MEOrdersAtPrice *getOrdersAtPrice(price priceLevel) {

            return ordersAtPriceHashMap.at(priceToIndex(priceLevel));

        }

        /**
         * @brief if a price level of priceVal already exists 
         *        returns the (priority of the last order at that price + 1)
         *        if this order is first at that price level returns 1
         * @param priceVal
         * @return (priority of the last order + 1) or 1
         */
        priority getNextPriority(price priceVal) noexcept {

            // getting orders at this price level
            const auto ordersAtPrice = getOrdersAtPrice(priceVal);

            // this order is first at its price level -> return 1
            if(!ordersAtPrice) 
                return 1UL;

            else
                return (ordersAtPrice -> firstMeOrder -> _prevOrder -> _prio) + 1;

        }

        /**
         * @brief adds order to the list of orders
         *        at the certain price level
         * @param &order is the refernce to the order instance we want to insert 
         *        to the book
         */
        void addOrder(MEOrder *order) {

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

                newOrdersAtPrice -> firstMeOrder = order;

                addOrdersAtPrice(newOrdersAtPrice);

            }

            // the order price level is already present in the order book
            else {

                auto firstOrder = ordersAtPrice -> firstMeOrder;

                firstOrder -> _prevOrder -> _nextOrder = order;

                order -> _prevOrder = firstOrder -> _prevOrder;

                order -> _nextOrder = firstOrder;

                firstOrder -> _prevOrder = order;


            }

        }

        /**
         * @brief removes order from the book (from the certain price level)
         */
        void removeOrder(MEOrder *order) noexcept {

            // getting linked list of orders on certain price level
            auto ordersAtPrice = getOrdersAtPrice(order -> _price);

            // if we have only one order in the linked list (one element in ring)
            if (order -> _prevOrder == order) {

                // removing the entire price level
                removeOrdersAtPrice(order -> _meOrderSide, order -> _price);

            }

            // remove the order from the linked list 
            else {

                const auto prevOrder = order -> _prevOrder;
                const auto nextOrder = order -> _nextOrder;

                // severing the connections between removed order
                // and its neighbors 
                prevOrder -> _nextOrder = nextOrder;
                nextOrder -> _prevOrder = prevOrder;

                if (ordersAtPrice -> firstMeOrder == order)
                    ordersAtPrice -> firstMeOrder = nextOrder;

                order -> _prevOrder = order -> _nextOrder = nullptr;

            }

            // removing order from the hashmap
            // (not implemented yet) !!!!!!!!!!!!!!!!!!!!!

            // releasing the slot in the mempool
            _ordersPool.deallocate(order);

        }

        /**
         * @brief executes a deal between an incoming order and the order that is already in the book (passive order)
         * @param tickerId
         * @param clientId is the client that sent the aggressive order
         * @param sideVal buy/sell
         * @param clientOrderId id of the order on the client-side
         * @param newMarketOrderId id of the order within the trading system (that will be unique
         *                         exactly in the trading system)
         * @param *matchingOrder is the pointer to the order that we match with. It is a head
         *        of the FIFO sequence of a certain price level
         * @param *leavesQty how many units left on the aggressive side after the match 
         * 
         */
        void match(ticker_id tickerId, client_id clientId, side sideVal, order_id clientOrderId, 
            order_id newMarketOrderId, MEOrder *matchingOrder, quantity *leavesQty) noexcept;

        /**
         * @brief checks if there is a cross between the aggressive order 
         *        and a passive order. And checks if they match fully or partially
         * @return 
         */
        quantity checkForMatch(client_id clientId, order_id orderId, ticker_id tickerId, 
            side sideVal, price priceVal, quantity q, order_id newOrderId) noexcept;

    };

}