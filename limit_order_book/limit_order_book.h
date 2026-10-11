#include "md_protocol.h"
#include "matching_engine.h"
#include "logger.h"
#include "mem_pool.h"
#include "me_order_struct.h"

namespace exchange {

    class MEOrderBook {
    public:

        explicit MEOrderBook(ticker_id tickerId, MatchingEngine *MatchingEngine, Logger *logger);

        ~MEOrderBook();

        // removing move and copy opeartors
        MEOrderBook(const MEOrderBook&)              = delete;
        MEOrderBook &operator = (const MEOrderBook&) = delete;
        MEOrderBook(MEOrderBook &&)                  = delete;
        MEOrderBook &operator = (MEOrderBook &&)     = delete; 

        // fucntion 
        auto add(client_id clientId, 
            order_id clientOrderId, 
            ticker_id tickerId, 
            side sideVal, 
            price priceVal, 
            quantity qty) noexcept -> void; 

        // function
        auto cancel(client_id clientId, order_id orderId, ticker_id tickerID) noexcept -> void;

    private:

        // Memory Pool where MEOrdersAtPrice are stored
        // we have two separate sequences for bids and for asks
        MemoryPool<MEOrdersAtPrice> ordersAtPricePool;

        MEOrdersAtPrice *bidsByPrice = nullptr;
        MEOrdersAtPrice *asksByPrice = nullptr;

    };

}