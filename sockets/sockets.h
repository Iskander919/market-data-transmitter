#pragma once

#include <arpa/inet.h> // this header declares functions that work with network
#include <fcntl.h> // this header declares functions that work with files
#include <ifaddrs.h> // decalres struct of network interface
#include <iostream>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "logger.h"

class Sockets {
public:

    // Sockets();

    const int maxTcpBacklog = 1024;

    // function that creates
    auto createSocket(const std::string &tIp, 
        const std::string &interface,
        int port,
        bool isUdp,
        bool isBlocking,
        bool isListening,
        int ttl,
        bool needsTimestamp,
        Logger &logger) -> int;

    // this function converts network interfaces 
    // represented in string form 
    // to the form that can be used by low-level socket routines
    auto getInterfaceIp(const std::string &iface) -> std::string;

    ssize_t udpSend(int fileDescriptor, 
        char* data, 
        size_t size, 
        int port,
        const std::string &ip);

private:

    // this function sets the socket to be non-blocking
    // it returns true if the socket was successfully set to 
    // be non blocking or has been already non-blocking
    auto setNonBlocking(int fileDescriptor) -> bool;

    // this function turns off buffering of 
    // TCP packets
    auto setNoDelay(int fileDEscriptor) -> bool;

    // this function checks if 
    // socket operation is blocking or not
    auto wouldBlock() -> bool;

    // this functions 
    auto setTTL(int fileDescriptor, int ttl) -> bool;

    auto setMcastTTL(int fileDescriptor, int mcastTtl) -> bool;

    // function that allows to generate timestamps
    // when a packet hits the socket
    auto setTimestamp(int fileDescriptor) -> bool;



};

