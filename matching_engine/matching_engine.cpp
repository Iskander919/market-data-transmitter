#include "matching_engine.h"

/**
 * @brief constructor of the matching engine class 
 * @param *marketUpdateLFQueue is a pointer to the insatnce of the lock-free queue
 *        that stores marketDataTypeDef structs
 * @param *clientRequestLFQueue is a pointer to the instance of the lock-free queue
 *        that stores clientResponseTypeDef structs that come from client-side to server-side
 * @param *clientResponseLFQueue is a pointer to the instance of the lock-free queue
 *        that stores clientResponseTypeDef structs that come from server-side to client-side
 * @return none
 */
exchange::MatchingEngine::MatchingEngine(exchange::MarketUpdateLFQueue *marketUpdateLfQueue, 
    exchange::ClientReqLFQueue *clientRequestLFQueue, 
    exchange::CLientResponseLFQueue *clientResponseLFQueue, 
    Logger *logger) : 
    _marketUpdateLfQueue(marketUpdateLfQueue), 
    _isRunning(false), 
    _logger(logger) 
    {

        if(_logger == nullptr) {

            std::cerr << "Logger instance is nullptr\n";
        }
        
        _logger -> log("Create instance of matching engine\n");

        #if DEBUG_MODE
        std::cout << "Created instance of matching engine" << std::endl;
        #endif

}

/**
 * @brief
 * @param none
 * @return none
 */
exchange::MatchingEngine::~MatchingEngine() {

    _matchingEngineThread.join();

    #if DEBUG_MODE
    std::cout << "Destroyed ME instance" << std::endl;
    #endif

}

/**
 * @brief creates and starts Matching Engine thread
 * @param none
 * @return none
 */
void exchange::MatchingEngine::start() {

    _isRunning = true;

    _logger -> log("Start matching engine\n");

    _matchingEngineThread = std::thread([this](){

        // processing data and put result to the queue
        // we will put some random data to the queue
        this -> processMarketData();

    });

    #if DEBUG_MODE
    std::cout << "Started ME thread" << std::endl;
    #endif

}

void exchange::MatchingEngine::stop() {

    _isRunning = false;

    #if DEBUG_MODE
    std::cout << "Stopped ME thread" << std::endl;
    #endif

}

void exchange::MatchingEngine::processMarketData() {

    while(_isRunning || _marketUpdateLfQueue -> size()) {

        // test boilerplate code
        #if DEBUG_MODE
        std::cout << "Processing market data" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        #endif

    } 

}
