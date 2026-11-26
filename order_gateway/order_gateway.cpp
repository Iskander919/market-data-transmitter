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

        // we set EPOLLIN since we make a listening socket and we 
        // ready to accept the incoming connection
        _event.events  = EPOLLIN;
        _event.data.fd = socketFileDesc;

        // register socketFileDesc in epoll 
        // we pass the EPOLL_CTL_ADD flag cause' we add new FD to the epoll
        int registerStatus = epoll_ctl(epollFd, EPOLL_CTL_ADD, socketFileDesc, &_event);

        if(registerStatus < 0) {

            std::cerr << "OrderGateway::epollConfigure:adding socket file descriptor in epoll failed" << std::endl;
            _logger -> log("OrderGateway::epollConfigure:adding socket file descriptor in epoll failed\n");

            close(epollFd);

            return -1;

        }

        return epollFd;

    }

    /**
     * @brief cyclically checks for incoming connections, accepts them, 
     *     pushes requests to the queue and extracts responses from the queue
     */
    void OrderGateway::runGateway() {

        if(_epollFd < 0) {

            std::cerr << "OrderGateway::runGateway:_epollFd < 0. Aborting application" << std::endl;
            _run = false;
            std::abort();

        }

        else if(_socketFd < 0) {

            std::cerr << "OrderGateway::runGateway:_socketFd < 0. Aborting application" << std::endl;
            _run = false;
            std::abort();

        }

        while(_run) {

            // getting number of file descriptors that have data to read
            int readyFds = epoll_wait(_epollFd, eventsArr.data(), MAX_EVENTS, EPOLL_TIMEOUT);

            if(readyFds == 0)
                continue;

            else if (readyFds <  0) {

                // got an interrupt
                if(errno == EINTR)
                    continue;

                std::cerr << "OrderGateway::runGateway: epoll_wait returned -1. Aboritng" << std::endl;
                _run = false;
                std::abort();

            }

            // processing all available descriptors
            for(int i = 0; i < readyFds; ++i) {

                const int fd = eventsArr[i].data.fd;

                // getting a bit mask to get an event 
                const uint32_t mask = eventsArr[i].events;

                // we got an event on the listening socket
                if(fd == _socketFd) { 

                    // if we get a error with epoll or connection failure we stop the order gateway
                    // EPOLLERR stands for the file descriptor error 
                    // EPOLLHUP stands for the connection break
                    if(mask & (EPOLLERR | EPOLLHUP)) {

                        _run = false;
                        _logger -> log("OrderGateway::runGateway: got a EPOLLERR or EPOLLHUP\n");
                        std::cerr << "OrderGateway::runGateway: got a EPOLLERR or EPOLLHUP" << std::endl;

                        std::abort();

                    }

                    // we got a new data on the listening socket, thus we have new clients
                    // whose connections we are gonna accept
                    if(mask & EPOLLIN) {

                        if(acceptConnections() == -1)
                            break;

                    }

                    continue;


                }

                // handleClientEvents();

            }

            if(!_run)
                break;

            // drainCLientResponses();
 
        }

    }

    /**
    * @brief registers new connections in the epoll. 
    *        Returns file descriptor of the accepted connection, in case 
    *        of fault returns -1
    */
    int OrderGateway::acceptConnections() {

        int clientFd = -1;

        // accepting one connection (we set NONBLOCK to make the socket non-blocking and
        // CLOEXEC to make this fd to be closed if this process calls exec())
        do {
            clientFd = accept4(_socketFd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        }

        while(clientFd == -1 && errno == EINTR);

        if(clientFd == -1) {

            return -1;

        }

        // disabling Nagles's algorithm
        const int one = 1;
        if(setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) == -1) {

            std::cerr << "OrderGateway::acceptConnections(): TCP_NODELAY failed" << std::endl;
            close(clientFd);
            return -1;

        }

        // adding the fd to the epoll
        epoll_event event{};
        // we register this socket fd in epoll. The epoll will be raised
        // if there is incoming data or the connection was broken
        event.events  = EPOLLIN | EPOLLRDHUP;
        event.data.fd = clientFd;

        if(epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &event) < 0) {

            std::cerr << "OrderGateway::acceptConnections(): EPOLL_CTL_ADD failed" << std::endl;
            close(clientFd);
            return -1;

        }

        Connection connection;
        connection.fd = clientFd; 
        connection.sendBuf.reserve(SEND_VECT_SIZE);
        connection.receivedBuf.reserve(RECV_VECT_SIZE); 

        // adding the connection to the map
        try{_connectionsMap.try_emplace(clientFd); } 

        catch (std::exception ex) {

            std::cerr << "OrderGateway::acceptConnections(): adding to _connectionsMap failed" << std::endl;
            // de-register in the epoll
            epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, nullptr);

            close(clientFd);
            return -1;

        }

        _connectionsMap.at(clientFd) = connection;

        return clientFd;

    }

    void OrderGateway::handleClientEvents(int fd, uint32_t mask) {

        // Соединение могло быть удалено при предыдущей обработке.
        if (_connectionsMap.find(fd) == _connectionsMap.end()) {
            return;
        }

        if (mask & EPOLLERR) {
            int socketError = 0;
            socklen_t length = sizeof(socketError);

            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &socketError, &length) == -1) {
                std::perror("OrderGateway::handleClientEvents: SO_ERROR");
            } else if (socketError != 0) {
                std::cerr << "OrderGateway::handleClientEvents: fd=" << fd 
                    << ": " << std::strerror(socketError) << '\n';
            }

            closeConnection(fd);
            return;
        }

        // При завершении соединения в сокете ещё могут оставаться
        // непрочитанные данные. Сначала пытаемся их обработать.
        if (mask & (EPOLLIN | EPOLLRDHUP | EPOLLHUP)) {
            if (!receiveClientRequests(fd)) {
                closeConnection(fd);
                return;
            }
        }

        // Полное завершение соединения.
        if (mask & EPOLLHUP) {
            closeConnection(fd);
            return;
        }

        // Сокет готов принять ещё часть исходящих байтов.
        if (mask & EPOLLOUT) {
            if (!sendClientResponses(fd)) {
                closeConnection(fd);
                return;
            }
        }
    }

    /**
     * @brief
     */
    bool OrderGateway::receiveClientRequests(int fd) { return false; }

    /**
     * @brief
     */
    bool OrderGateway::sendClientResponses(int fd) { return false; }

    /**
     * @brief
     */
    void OrderGateway::closeConnection(int fd) { return; }
}