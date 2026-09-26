#include "matching_engine.h"
#include "logger.h"
#include "iostream"

int main() {

    std::string filename = "log_matching_engine.txt";

    exchange::ClientReqLFQueue      clientReqLFQ();
    exchange::CLientResponseLFQueue clientResponseLFQ();
    exchange::MarketUpdateLFQueue   marketUpdateLFQ();

    Logger logger(filename);


    // exchange::MatchingEngine me(&marketUpdateLFQ, &clientReqLFQ, &clientResponseLFQ, &logger);

    return 0;

}