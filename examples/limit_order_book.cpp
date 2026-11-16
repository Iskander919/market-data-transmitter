
#include "limit_order_book.h"
#include "matching_engine.h"
#include <chrono>
#include <thread>
#include <memory>


exchange::ClientReqLFQueue      clientReqLFQ(256);
exchange::CLientResponseLFQueue clientResponseLFQ(256);
exchange::MarketUpdateLFQueue   marketUpdateLFQ(256);

int main() {

    std::string filename = "log_matching_engine.txt";

    Logger logger(filename);

    exchange::MatchingEngine engine(&marketUpdateLFQ, &clientReqLFQ, &clientResponseLFQ, &logger);

    exchange::ticker_id ticker = 1;

    std::unique_ptr<exchange::MEOrderBook> bookPtr = 
        std::make_unique<exchange::MEOrderBook>(ticker, &engine, &logger);

    return 0;
    
}