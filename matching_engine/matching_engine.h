#pragma once

#include <atomic>
#include <thread>

#include "lock_free_queue.h"
#include "md_protocol.h"
#include "order_gateway.h"


namespace exchange {

    // data that comes to market data publisher
    typedef LockFreeQueue<exchange::marketDataTypeDef> MarketUpdateLFQueue;

    // data that comes from order gateway
    typedef LockFreeQueue<exchange::clientResponseTypeDef> ClientReqLFQueue;

    // data that comes to order gateway
    typedef LockFreeQueue<exchange::clientResponseTypeDef> CLientResponseLFQueue;


    class MatchingEngine {
    public:

        explicit MatchingEngine(MarketUpdateLFQueue *marketUpdateLfQueue);

        ~MatchingEngine();

        // starts matching engine thread
        void start();

        // stops matching engine thread
        void stop();

    private:

        MarketUpdateLFQueue *_marketUpdateLfQueue;

        std::atomic<bool> _isRunning;

        std::thread _matchingEngineThread;

    };

}