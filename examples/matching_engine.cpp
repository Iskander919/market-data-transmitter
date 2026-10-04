#include "matching_engine.h"
#include "logger.h"
#include "iostream"

int main() {

    std::string filename = "log_matching_engine.txt";

    exchange::ClientReqLFQueue      clientReqLFQ(256);
    exchange::CLientResponseLFQueue clientResponseLFQ(256);
    exchange::MarketUpdateLFQueue   marketUpdateLFQ(256);

    Logger logger(filename);

    exchange::MatchingEngine me(&marketUpdateLFQ, &clientReqLFQ, &clientResponseLFQ, &logger);

    me.start();

    return 0;

}