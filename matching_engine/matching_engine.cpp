#include "matching_engine.h"
#include "limit_order_book.h"

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
    _clientReqLFQueue(clientRequestLFQueue),
    _clientResponseLFQueue(clientResponseLFQueue),
    _isRunning(false), 
    _logger(logger) 
    {

        if(_logger == nullptr) {

            std::cerr << "Logger instance is nullptr\n";
            std::abort();

        }
        
        _logger -> log("ME::MatchingEngine(): Create instance of matching engine\n");

        #if DEBUG_MODE
        std::cout << "Created instance of matching engine" << std::endl;
        #endif

        // we have separate order book for every ticker, thus we need 
        // to allocate memory for every book in the heap
        for(ticker_id i = 0; i < ME_MAX_TICKERS; ++i) {

            orderBookArray[i] = new MEOrderBook(i, this, _logger);

        }

}

/**
 * @brief
 * @param none
 * @return none
 */
exchange::MatchingEngine::~MatchingEngine() {

    _isRunning = false;

    if(_matchingEngineThread.joinable())
        _matchingEngineThread.join();

    // deallocating the order books
    for (ticker_id i = 0; i < ME_MAX_TICKERS; ++i) {

        delete orderBookArray[i];

    }

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

    _matchingEngineThread = std::thread([this](){

        run();

    });

    _logger -> log("ME::start(): started matching engine thread\n");

}

void exchange::MatchingEngine::stop() {

    _isRunning = false;

    if(_matchingEngineThread.joinable())
        _matchingEngineThread.join();

    _logger -> log("ME::stop(): stopped ME thread\n");

}

/**
 * @brief sends response to the client's request (through putting it into the queue)
 */
void exchange::MatchingEngine::sendClientResponse(const clientResponseTypeDef &response) { 

    clientResponseTypeDef *slot = _clientResponseLFQueue -> getNextSlotToWrite();
    (*slot) = response;

    _clientResponseLFQueue -> updateWriteIndex();

}

/**
 * @brief sends incremental update of the book (through putting it into the queue)
 */
void exchange::MatchingEngine::sendMarketUpdate(const marketDataTypeDef &marketUpdate) { 

    marketDataTypeDef *slot = _marketUpdateLfQueue -> getNextSlotToWrite();
    (*slot) = marketUpdate;

    _marketUpdateLfQueue -> updateWriteIndex();

}

/**
 * @brief accepts the client's request and raises the corresponding 
 *        methods in the order book
 */
void exchange::MatchingEngine::processClientRequest(const clientRequestTypeDef &clientRequest) noexcept {

    // getting a pointer to the order book for the ticker of client request
    if(UNLIKELY(clientRequest.tickerId >= ME_MAX_TICKERS)) {

        clientResponseTypeDef response;

        if(clientRequest.requestType == clientRequestTypeEnum::CANCEL) {

            response = clientResponseTypeDef(clientResponseTypeEnum::CANCEL_REJECTED, clientRequest.clientId, clientRequest.tickerId,
                ORDER_ID_INVALID, clientRequest.orderId, side::INVALID, clientRequest.priceValue, clientRequest.qty, 
                QUANTITY_INVALID);

        }

        else if(clientRequest.requestType == clientRequestTypeEnum::NEW) {

            response = clientResponseTypeDef(clientResponseTypeEnum::REJECTED, clientRequest.clientId, clientRequest.tickerId,
                ORDER_ID_INVALID, clientRequest.orderId, side::INVALID, clientRequest.priceValue, clientRequest.qty, 
                QUANTITY_INVALID);

        }

        sendClientResponse(response);

        std::cerr << "ME::processClientRequest: Ticker ID is too big" << std::endl;
        return;

    }

    MEOrderBook *book = orderBookArray.at(clientRequest.tickerId);

    switch(clientRequest.requestType){

        case clientRequestTypeEnum::INVALID:

            std::cerr << "ME::processClientRequest(): got an INVALID request" << std::endl;

        break;

        case clientRequestTypeEnum::NEW:

            book -> add(clientRequest.clientId, clientRequest.orderId, clientRequest.tickerId, 
                clientRequest.sideValue, clientRequest.priceValue, clientRequest.qty);

        break;

        case clientRequestTypeEnum::CANCEL:

            book -> cancel(clientRequest.clientId, clientRequest.orderId, clientRequest.tickerId);

        break;

        default:

            std::cerr << "ME::processClientRequest(): received an invalid request from the client" << std::endl;
            
        break;

    }

}

/**
 * @brief main cycle of the matching engine: it reads the queue of clients'
 *        requests and puts responses and market updates to corresponding queues
 */
void exchange::MatchingEngine::run() noexcept { 

    while(_isRunning || (_clientReqLFQueue -> getNextSlotToRead() != nullptr)) {

        // reading requests from the client's requests queue
        const clientRequestTypeDef *request = _clientReqLFQueue -> getNextSlotToRead();

        if(request == nullptr) {

            // _logger -> log("ME::run(): got an empty request from the queue\n");
            continue;

        }

        processClientRequest(*request);

        _clientReqLFQueue -> updateReadIndex();

    }

}
