#pragma once

#include <atomic>
#include <iostream>
#include <thread>
#include <chrono>

#include "lock_free_queue.h"
#include "md_protocol.h"
#include "order_gateway.h"
#include "logger.h"


namespace exchange {

    // data that comes to market data publisher
    typedef LockFreeQueue<exchange::marketDataTypeDef> MarketUpdateLFQueue;

    // data that comes from order gateway
    typedef LockFreeQueue<exchange::clientResponseTypeDef> ClientReqLFQueue;

    // data that comes to order gateway
    typedef LockFreeQueue<exchange::clientResponseTypeDef> CLientResponseLFQueue;


    class MatchingEngine {
    public:

        explicit MatchingEngine(exchange::MarketUpdateLFQueue *marketUpdateLfQueue, 
                                exchange::ClientReqLFQueue *clientRequestLFQueue, 
                                exchange::CLientResponseLFQueue *clientResponseLFQueue, 
                                Logger *logger);

        ~MatchingEngine();

        // starts matching engine thread
        void start();

        // stops matching engine thread
        void stop();

    private:

        MarketUpdateLFQueue *_marketUpdateLfQueue;

        ClientReqLFQueue    *_clientReqLFQueue;

        CLientResponseLFQueue *_clientResponseLFQueue;

        std::atomic<bool> _isRunning;

        std::thread _matchingEngineThread;

        void processMarketData();

        void processClientRequests();

        // pointer to the Logger insatnce
        Logger *_logger;

    };

}