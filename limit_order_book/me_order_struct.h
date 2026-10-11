#include <array>
#include "md_protocol.h"

namespace exchange {

    constexpr size_t ME_MAX_ORDER_IDS = 1024 * 1024;

    struct MEOrder {

        ticker_id _tickerID      = TICKER_ID_INVALID;
        client_id _clientID      = CLIENT_ID_INVALID;
        order_id  _clientOrderID = ORDER_ID_INVALID;
        order_id  _marketOrderID = ORDER_ID_INVALID;
        side _meOrderSide        = side::INVALID;
        priority _prio           = PRIORITY_INVALID;

        // since MEOrder is a doubly linked list, we have 
        MEOrder *_nextOrder = nullptr;
        MEOrder *_prevOrder = nullptr;

        // telling compiler to generate default constructor 
        // (this is needed to use MEOrder with MEMPool )
        MEOrder() = default;

        MEOrder(ticker_id tickerId, 
            client_id clientId, 
            order_id clientOrderID, 
            order_id marketOrderID, 
            side meOrderSide, 
            priority prio) : 

            _tickerID(tickerId),
            _clientID(clientId),
            _clientOrderID(clientOrderID),
            _marketOrderID(marketOrderID),
            _meOrderSide(meOrderSide),
            _prio(prio) { }

    };

    // OrdersHashMap is the hash map that stores OrderID as a key 
    // and reference to the MEOrder structure as a value
    typedef std::array<MEOrder*, ME_MAX_ORDER_IDS> OrderHashMap;

    // MEOrdersAtPrice struct represents each price level
    struct MEOrdersAtPrice {

        side  _sideVal   = side::INVALID;
        price _priceVal = PRICE_INVALID;

        MEOrder *firstMeOrder = nullptr;

        MEOrdersAtPrice() = default;

        MEOrdersAtPrice(side sideVal, price priceVal, MEOrdersAtPrice *nextEntry, MEOrdersAtPrice *prevEntry) : 
        _sideVal(sideVal), 
        _priceVal(priceVal), 
        _prevEntry(prevEntry),
        _nextEntry(nextEntry) {}

        MEOrdersAtPrice *_nextEntry = nullptr;
        MEOrdersAtPrice *_prevEntry = nullptr;

    };

};