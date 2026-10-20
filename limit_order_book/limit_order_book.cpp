#include "limit_order_book.h"

namespace exchange {

    MEOrderBook::MEOrderBook(ticker_id tickerId, MatchingEngine *matchingEngine, Logger *logger) : 
     _matchingEngine(matchingEngine), _bookTickerId(tickerId), _logger(logger) { }

    MEOrderBook::~MEOrderBook() { }

    /**
     * @brief checks if the order matches some of the orders in the book. If it doesn't
     *        adds the order to the order book 
     * @param clientId
     * @param clientOrderId
     * @param tickerId
     * @param sideVal
     * @param priceVal
     * @param qty
     * @return none
     */
    auto MEOrderBook::add(client_id clientId, 
            order_id clientOrderId, 
            ticker_id tickerId, 
            side sideVal, 
            price priceVal, 
            quantity qty) noexcept -> void {

                const auto newOrderId = generateNewMarketOrderId();

                // constructing response to client after "add" request
                _clientResponse = clientResponseTypeDef {clientResponseTypeEnum::ACCEPTED, 
                    clientId,  
                    tickerId,
                    clientOrderId,
                    newOrderId, 
                    sideVal, 
                    priceVal, 
                    0,
                    qty};

                _matchingEngine -> sendClientResponse(_clientResponse);

                // checks whether a partial or complete match can be made.
                // This variable stores the remaining quantity of this order after
                // the matching event. For orders that fully executre leavesQuantity = 0.
                quantity leavesQuantity = checkForMatch(clientId, 
                    clientOrderId, 
                    tickerId, 
                    sideVal,
                    priceVal,
                    qty, 
                    newOrderId);

                // if the order was not executed completely, add it to the book
                if (LIKELY(leavesQuantity)) {

                    const auto priority = getNextPriority(priceVal);

                    // allocating new slot in the memory pool for 
                    // this order
                    auto order = _ordersPool.allocate(tickerId, 
                        clientId, 
                        clientOrderId,
                        newOrderId, 
                        sideVal, 
                        priceVal, 
                        leavesQuantity, 
                        priority,
                        nullptr, 
                        nullptr);

                    addOrder(order);

                    _marketUpdate = marketDataTypeDef{...}

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

}