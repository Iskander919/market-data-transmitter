
#include "limit_order_book.h"
#include "matching_engine.h"
#include <chrono>
#include <thread>


exchange::ClientReqLFQueue      clientReqLFQ(256);
exchange::CLientResponseLFQueue clientResponseLFQ(256);
exchange::MarketUpdateLFQueue   marketUpdateLFQ(256);

int main() {

    std::string filename = "log_matching_engine.txt";

    Logger logger(filename);

    exchange::MatchingEngine engine(&marketUpdateLFQ, &clientReqLFQ, &clientResponseLFQ, &logger);

    exchange::ticker_id ticker = 1;

    exchange::MEOrderBook book(ticker, &engine, &logger);

    return 0;
    
}