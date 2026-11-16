#pragma once

#include <atomic>
#include <iostream>
#include <thread>
#include <chrono>

#include "lock_free_queue.h"
#include "md_protocol.h"
#include "order_gateway.h"
#include "logger.h"

#define DEBUG_MODE 1

// defining maximum numbers of ticker that can exist in the trading system
constexpr size_t ME_MAX_TICKERS = 8;

namespace exchange {

    class MEOrderBook;

    // data that comes to market data publisher
    typedef LockFreeQueue<exchange::marketDataTypeDef> MarketUpdateLFQueue;

    // data that comes from order gateway
    typedef LockFreeQueue<exchange::clientRequestTypeDef> ClientReqLFQueue;

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

        void sendClientResponse(const clientResponseTypeDef &response);

        void sendMarketUpdate(const marketDataTypeDef &marketData);

        void processClientRequest(const clientRequestTypeDef &request) noexcept;

        void run() noexcept;

    private:

        MarketUpdateLFQueue *_marketUpdateLfQueue;

        ClientReqLFQueue    *_clientReqLFQueue;

        CLientResponseLFQueue *_clientResponseLFQueue;

        std::atomic<bool> _isRunning;

        std::thread _matchingEngineThread;

        // pointer to the Logger insatnce
        Logger *_logger;

        std::array<MEOrderBook*, ME_MAX_TICKERS> orderBookArray;

    };

}