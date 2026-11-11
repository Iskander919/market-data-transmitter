#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <bit>

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

namespace exchange {

    // order_id is
    typedef uint64_t order_id;

    // define maximum possible value of order_id
    constexpr uint64_t ORDER_ID_INVALID = std::numeric_limits<order_id>::max();

    // for example AAPL etc.
    typedef uint32_t ticker_id;
    constexpr ticker_id TICKER_ID_INVALID = std::numeric_limits<ticker_id>::max();

    typedef uint32_t client_id;
    constexpr client_id CLIENT_ID_INVALID = std::numeric_limits<client_id>::max();

    // price is used to capture price on orders 
    typedef int64_t price;
    constexpr int64_t PRICE_INVALID = std::numeric_limits<price>::max();

    typedef uint64_t quantity;
    constexpr uint64_t QUANTITY_INVALID = std::numeric_limits<quantity>::max();

    typedef uint64_t priority;
    constexpr uint64_t PRIORITY_INVALID = std::numeric_limits<priority>::max();

    enum class side : int8_t {

        INVALID =  0,
        BUY     =  1,
        SELL    = -1

    };

    // ADD - we have a new order in the book
    // MODIFY - the quantity of the order has been changed
    // TRADE - is triggered after every match()
    enum class MarketUpdateType : uint8_t {

        INVALID = 0,
        ADD     = 1,
        MODIFY  = 2, 
        CANCEL  = 3, 
        TRADE   = 4

    };

    struct marketDataTypeDef {

        MarketUpdateType marketUpdateType = MarketUpdateType::INVALID;

        order_id  orderId        = ORDER_ID_INVALID;
        ticker_id tickerId       = TICKER_ID_INVALID;
        price     priceValue     = PRICE_INVALID;
        quantity  quant          = QUANTITY_INVALID;
        priority  priorityValue  = PRIORITY_INVALID;
        side      sideValue      = side::INVALID;

        marketDataTypeDef() {}

        marketDataTypeDef(MarketUpdateType _updateType, order_id _orderId, ticker_id _tickerId, price _priceVal,
                          quantity _quant, priority _priorityValue, side _sideValue) :  

                          marketUpdateType(_updateType), orderId(_orderId), tickerId(_tickerId),
                          priceValue(_priceVal), quant(_quant), priorityValue(_priorityValue), sideValue(_sideValue)
                          { }

    };

    constexpr uint32_t marketDataStructSize = 40;

}


class MarketDataProtocolUtils {
public:

    static std::array<uint8_t, exchange::marketDataStructSize> 
    combineByteSequence(const exchange::marketDataTypeDef &mdStruct);

    static inline std::string orderIdToString(exchange::order_id orderId) {

        if(UNLIKELY(orderId == exchange::ORDER_ID_INVALID)) 
            return "INVALID";
        
        return std::to_string(orderId);

    }

    static inline std::string tickerIdToString(exchange::ticker_id tickerId) {

        if(UNLIKELY(tickerId == exchange::TICKER_ID_INVALID)) 
            return "INVALID";
        
        return std::to_string(tickerId);

    }

    static inline std::string clientIdToString(exchange::client_id clientId) {

        if(UNLIKELY(clientId == exchange::CLIENT_ID_INVALID)) 
            return "INVALID";
        
        return std::to_string(clientId);

    }

    static inline std::string priceIdToString(exchange::price pr) {

        if(UNLIKELY(pr == exchange::PRICE_INVALID)) 
            return "INVALID";
        
        return std::to_string(pr);

    }

    static inline std::string quantityToString(exchange::quantity quant) {

        if(UNLIKELY(quant == exchange::QUANTITY_INVALID)) 
            return "INVALID";
        
        return std::to_string(quant);

    }

    static inline std::string priorityToString(exchange::priority pr) {

        if(UNLIKELY(pr == exchange::PRIORITY_INVALID)) 
            return "INVALID";
        
        return std::to_string(pr);

    }

    static inline std::string sideToString(exchange::side sid) {

        switch(sid) {

            case exchange::side::BUY:
            return "BUY";
            break;

            case exchange::side::SELL:
            return "SELL";
            break;

            case exchange::side::INVALID:
            return "INVALID";
            break;
            
        }

    }

};
