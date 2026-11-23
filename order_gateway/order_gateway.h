#pragma once

#include "md_protocol.h"

#include <array>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <sys/epoll.h>
#include "sockets.h"
#include <unistd.h>
#include <unordered_map>

namespace exchange {

    enum class clientResponseTypeEnum : uint8_t {

        INVALID         = 0,
        ACCEPTED        = 1,
        CANCELED        = 2,
        FILLED          = 3,
        CANCEL_REJECTED = 4,
        REJECTED        = 5

    };

    enum class clientRequestTypeEnum : uint8_t {

        INVALID  = 0,
        NEW      = 1,
        CANCEL   = 2

    };

    // (MEClientResponse according to the book)
    // this struct contains data that exchange transfers to clients 
    // in response to their orders
    struct clientResponseTypeDef {

        explicit clientResponseTypeDef();

        explicit clientResponseTypeDef(
            clientResponseTypeEnum _responseStatus,
            client_id _clientId,
            ticker_id _tickerId,
            order_id _marketOrderId,
            order_id _clientOrderId,
            side _sideValue,
            price _priceValue,
            quantity _qty,
            quantity _leavesQty) : 

            responseStatus(_responseStatus),
            clientId(_clientId),
            tickerId(_tickerId),
            marketOrderId(_marketOrderId),
            clientOrderId(_clientOrderId),
            sideValue(_sideValue),
            priceValue(_priceValue),
            qty(_qty),
            leavesQty(_leavesQty) { }

        clientResponseTypeEnum responseStatus;
        client_id clientId;
        ticker_id tickerId;
        order_id  marketOrderId;
        order_id  clientOrderId;
        side      sideValue;
        price     priceValue;
        quantity  qty;
        quantity  leavesQty;

        std::string toString();

    };

    // (MEClientRequest according to the book)
    // this struct represents the requests that clients 
    // send to the exchange server 
    struct clientRequestTypeDef {

        explicit clientRequestTypeDef();

        clientRequestTypeEnum requestType;
        client_id clientId;
        ticker_id tickerId;
        order_id  orderId;
        side      sideValue;
        price     priceValue;
        quantity  qty;

        std::string toString();

    };

    class OrderGateway final {
    public:

        OrderGateway(std::string ip, std::string interface, int port, int ttl, Logger *logger);
        ~OrderGateway();

        static constexpr size_t CLIENT_RESPONSE_SIZE = 0;
        static constexpr size_t CLIENT_REQUEST_SIZE  = 0;

        /**
         * @brief sets _run as true and starts the _orderGatewayThread
         */
        void startOrderGatewayThread();

        /**
         * @brief sets _run flag as false  
         */
        void stopGatewayThread();

        /**
         * @brief gets client's request represented as byte flow and converts it to the clientRequestTypeDef
         */
     
        clientRequestTypeDef deserializeClientRequest(std::array<uint8_t, CLIENT_REQUEST_SIZE> &req);

        /**
         * @brief gets response as clientResponseTypeDef and returns it as a byte flow
         */
        std::array<uint8_t, CLIENT_RESPONSE_SIZE> serializeClientResponse(clientResponseTypeDef &response);

        /**
         * @brief inititalizes a new socket and returns its file descriptor
         */
        int initSocket(); 

        /**
         * @brief configures epoll for order gateway
         */
        int epollConfigure(int socketFileDesc);

        /**
         * @brief cyclically checks for incoming connections, accepts them, 
         *        pushes requests to the queue and extracts responses from the queue
         */
        void runGateway();

    private:

        Sockets sockets;

        std::string _ip; // tIp for createSocket()
        std::string _interface; // interface for createSocket() 
        int _port; // port for createSocket
        int _ttl; // ttl for createSocket

        // event instance for epoll
        epoll_event _event{};

        // file descriptors of the listening socket and epoll
        int _socketFd = -1;
        int _epollFd  = -1;

        // utils for the order gateway thread 
        std::atomic<bool> _run = false;
        std::thread _orderGatewayThread;

        Logger *_logger;

        // struct that stores info about client's connection
        struct Connection { };

        // maps for storing connections 
        // and matching between client_ids and file descriptors
        std::unordered_map<int, Connection> _connectionsMap = {};
        std::unordered_map<client_id, int> _clientIdToFd = {}; 

        static const uint32_t MAX_EVENTS    = 64;
        static const uint32_t EPOLL_TIMEOUT = 10;

        std::array<epoll_event, MAX_EVENTS> eventsArr = {};

    };

}