#include "order_gateway.h"

exchange::clientResponseTypeDef::clientResponseTypeDef() :  
    clientId(CLIENT_ID_INVALID), tickerId(TICKER_ID_INVALID), marketOrderId(ORDER_ID_INVALID), 
    clientOrderId(ORDER_ID_INVALID), sideValue(side::INVALID), priceValue(PRICE_INVALID), 
    qty(QUANTITY_INVALID), leavesQty(QUANTITY_INVALID) { }

std::string exchange::clientResponseTypeDef::toString() {

    return "";

}

exchange::clientRequestTypeDef::clientRequestTypeDef() : 
    requestType(clientRequestTypeEnum::INVALID), clientId(CLIENT_ID_INVALID), tickerId(TICKER_ID_INVALID), orderId(ORDER_ID_INVALID),
    sideValue(side::INVALID), priceValue(PRICE_INVALID), qty(QUANTITY_INVALID) { }

std::string exchange::clientRequestTypeDef::toString() {

    return "";

}

namespace exchange {

    OrderGateway::OrderGateway(std::string ip, std::string interface, int port, int ttl, Logger *logger) : _ip(ip), _interface(interface), 
        _port(port), _ttl(ttl), _logger(logger) { 

            if(_logger == nullptr) {

                std::cerr << "OrderGateway::OrderGateway:logger is nullptr" << std::endl;
                std::abort();

            }

        }
    OrderGateway::~OrderGateway() { }

    /**
     * @brief opens listening TCP non-blocking socket an returns its file descriptor
     */
    int OrderGateway::initSocket() {

        // opening listening socket 
        int socketFileDesc = sockets.createSocket(_ip, _interface, _port, false, false, true, _ttl, false, _logger);

        return socketFileDesc;

    }

    /**
     * @brief configures epoll for order gateway and returns epoll file descriptor
     */
    int OrderGateway::epollConfigure(int socketFileDesc) {

        // creating epoll instance and getting its file descriptor 
        // we pass EPOLL_CLOEXEC flag so the epollFd is closed when exec() is called
        // so this epoll instance will not be obtained by the child process
        const int epollFd = epoll_create1(EPOLL_CLOEXEC);

        if(epollFd < 0) {

            std::cerr << "OrderGateway::epollConfigure:error creating epoll file descriptor" << std::endl;
            _logger -> log("OrderGateway::epollConfigure:error creating epoll file descriptor\n");
            return -1;

        }

        epoll_event event{};

        // register socketFileDesc in epoll 
        // we pass the EPOLL_CTL_ADD flag cause' we add new FD to the epoll
        int registerStatus = epoll_ctl(epollFd, EPOLL_CTL_ADD, socketFileDesc, &event);

        if(registerStatus < 0) {

            std::cerr << "OrderGateway::epollConfigure:adding socket file descriptor in epoll failed" << std::endl;
            _logger -> log("OrderGateway::epollConfigure:adding socket file descriptor in epoll failed\n");

            close(epollFd);

            return -1;

        }

        return epollFd;

    }

}