#pragma once

#include "md_protocol.h"

#include <cerrno>
#include <cstdio>
#include <sys/epoll.h>
#include <unistd.h>
#include "sockets.h"

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

    private:

        Sockets sockets;

        std::string _ip; // tIp for createSocket()
        std::string _interface; // interface for createSocket() 
        int _port; // port for createSocket
        int _ttl; // ttl for createSocket

        // file descriptors of the listening socket and epoll
        int _socketFd = -1;
        int _epollFd  = -1;

        Logger *_logger;

    };

}