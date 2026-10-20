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
        auto add(client_id clientId, 
            order_id clientOrderId, 
            ticker_id tickerId, 
            side sideVal, 
            price priceVal, 
            quantity qty) noexcept -> void; 

        // function
        auto cancel(client_id clientId, order_id orderId, ticker_id tickerID) noexcept -> void;

        // funtion that prints the current 
        // contents of the limit order book
        auto print() noexcept -> void;

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

        order_id _nextMarketOrderId = 1;

        // keeps data for responding to client's
        // requests
        clientResponseTypeDef _clientResponse;

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
         * @brief adds new price level to the MEOrdersAtPrice
         */
        void addOrdersAtPrice(MEOrdersAtPrice *newOrderAtPrice) {

            // do something

        }

        /**
         * @brief
         * @param
         * @return
         */
        auto getOrdersAtPrice(price priceArg) -> MEOrdersAtPrice * {

            // return 

        }

        /**
         * @brief
         * @return 
         */
        auto checkForMatch(client_id clientId, order_id orderId, ticker_id tickerId, 
            side sideVal, price priceVal, quantity q, order_id newOrderId) noexcept -> quantity  {


                return 0;

        }

        /**
         * @brief returns reference to the first order at the demanded price
         * @param priceLevel is the desired price level
         * @return reference to the MEOrdersAtPrice member
         *         which contains reference to the order list of demanded price
         */
        MEOrdersAtPrice *getOrdersAtPriceLevel(price priceLevel) {


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
                MEOrdersAtPrice *newOrderAtPrice = _ordersAtPricePool.allocate(order -> _meOrderSide, 
                                                                               order -> _price, 
                                                                               order, 
                                                                               nullptr, 
                                                                               nullptr);

                addOrdersAtPrice(newOrderAtPrice);

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

    };

}