#include "matching_engine.h"

exchange::MatchingEngine::MatchingEngine(MarketUpdateLFQueue *marketUpdateLfQueue) 
: _marketUpdateLfQueue(marketUpdateLfQueue), _isRunning(false) { }

exchange::MatchingEngine::~MatchingEngine() {

    _matchingEngineThread.join();

}

void exchange::MatchingEngine::start() {

    _isRunning = true;

    _matchingEngineThread = std::thread([this](){

        // processing data and put result to the queue
        // we will put some random data to the queue


    });

}

void exchange::MatchingEngine::stop() {

    _isRunning = false;

}



