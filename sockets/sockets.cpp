#include "sockets.h"

/**
 * @brief function that returns IP-address of the network interface as a string
 * @param iface is a name of network interface ("wlan0", "eth0") represented in string
 * @return IP-address of the network interface
 */
std::string Sockets::getInterfaceIp(const std::string &iface) {

    // ifaddrs is data structure
    // that describes a single network interface
    // it contains fields:
    // -ifa_name (interface name)
    // -ifa_addr (interface IP address)
    // -ifa_next (pointer to the next node of the linked list)
    ifaddrs *ifaddr = nullptr;

    char buf[NI_MAXHOST] = {'\0'};

    // we transmit pointer to a pointer (getifaddrs**) and get list of the interfaces
    // via getifaddrs function
    if (getifaddrs(&ifaddr) != -1) {

        // traversing list 
        for(ifaddrs *ifa = ifaddr; ifa; ifa = ifa -> ifa_next) {

            // checking if the interface exists and has the address type of IPv4 (AF_INET)
            if ((ifa -> ifa_addr) && (ifa -> ifa_addr -> sa_family == AF_INET) && (iface == ifa -> ifa_name)) {

                // getting IP-address and writitng to the buf
                getnameinfo(ifa -> ifa_addr, sizeof(sockaddr_in), buf, sizeof(buf), NULL, 0, NI_NUMERICHOST);
                break;

            }
        }

        // deallocation of memory
        freeifaddrs(ifaddr);

    }
    return  buf;

}

/**
 * @brief this function sets a file (via fileDescriptor) to be non-blocking
 * @param fileDescriptor is file descriptor of the socket that needs to be set as non-blocking
 * @return true if the socket was successfully set to be non-blocking; false if opposite
 */
bool Sockets::setNonBlocking(int fileDescriptor) {

    // getting flags of this file 
    const int flags = fcntl(fileDescriptor, F_GETFL, 0);

    if(flags == -1) 
        return false;

    // if socket is already non-blocking, return true
    if(flags & O_NONBLOCK) 
        return true;

    // setting the socket to be non-blocking
    return (fcntl(fileDescriptor, F_SETFL, flags | O_NONBLOCK) != -1);

}

/**
 * @brief setNoDelay is the function that sets TCP_NODELAY option
 * @param fileDescriptor is the descriptor of the socket that needs to be set as non-delay
 * @return true if the operation was successful
 */
bool Sockets::setNoDelay(int fileDescriptor) {

    int one = 1;

    return (setsockopt(fileDescriptor, IPPROTO_TCP, TCP_NODELAY, 
        reinterpret_cast<void*>(&one), sizeof(one)) != -1);

}

/**
 * @brief function that checks if the socket operation would block
 * @param none
 * @return true if global errnos' have value EWOULDBLOCK or EINPROGRESS
 */
bool Sockets::wouldBlock() {

    return (errno == EWOULDBLOCK || errno == EINPROGRESS);

}

/**
 * @brief fucntion that sets time to live value
 * @param fileDescriptor is descriptor of the demanded socket
 * @param ttl is desired time to live value (maximum number of hops)
 * @return true if operation was successful
 */
bool Sockets::setTTL(int fileDescriptor, int ttl) {

    int res = setsockopt(fileDescriptor, IPPROTO_IP, IP_TTL, reinterpret_cast<void*>(&ttl), 
        sizeof(ttl) != -1);


    return res;

}

/**
 * @brief function that sets SO timestamp
 * @param fileDescriptor is descriptor of the socket  
 * @return true if operation was successful
 */
bool Sockets::setTimestamp(int fileDescriptor) {

    int one = 1;

    return (setsockopt(fileDescriptor, SOL_SOCKET, SO_TIMESTAMP, 
        reinterpret_cast<void *>(&one), sizeof(one)) != -1);

}

bool Sockets::setMcastTTL(int fileDescriptor, int mcastTtl) {

    return (setsockopt(fileDescriptor, IPPROTO_IP, IP_MULTICAST_TTL, 
        reinterpret_cast<void*>(&mcastTtl), sizeof(mcastTtl)) != -1);

}

/**
 * @brief
 * @param tIp
 * @param interface
 * @param port
 * @param isUdp
 * @param isBlocking
 * @param isListening
 * @param ttl
 * @param needsTimestamp
 * @return socketFileDescriptor
 */
int Sockets::createSocket(const std::string &tIp, 
    const std::string &interface,
    int port,
    bool isUdp,
    bool isBlocking,
    bool isListening,
    int ttl,
    bool needsTimestamp,
    Logger &logger) {

    std::string timeStr;

    // getting IP-address as a string
    const auto ip = tIp.empty() ? getInterfaceIp(interface) : tIp;

    logger.log("createSocket\n");

    addrinfo hints{};

    hints.ai_family = AF_INET;
    hints.ai_socktype = isUdp ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = isUdp ? IPPROTO_UDP : IPPROTO_TCP;
    hints.ai_flags = isListening ? AI_PASSIVE : 0;

    if(std::isdigit(ip.c_str() [0])) 
        hints.ai_flags |= AI_NUMERICHOST;
    
    else 
        hints.ai_flags |= AI_NUMERICSERV;

    addrinfo* result = nullptr;

    const int rc = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &result);

    // if rc returned error code, we failed to create a socket
    if (rc) 
        return -1;

    // making function call to create the socket
    int socketFileDescriptor = -1;
    int one = 1;
    
    for(addrinfo *rp = result; rp != nullptr; rp = rp -> ai_next) {

        socketFileDescriptor = socket(rp -> ai_family, rp -> ai_socktype, rp -> ai_protocol);

        // failed to get file descriptor of the socket 
        if(socketFileDescriptor == -1) {

            return -1;

        }
            

        if(!isBlocking) {

            // failed to set socket as non blocking
            if(!setNonBlocking(socketFileDescriptor)) {

                return -1;

            }
        }

        // failed to set TCP socket as no-delay
        if(!isUdp && !setNoDelay(socketFileDescriptor)) {
            return -1;
        }
        // connect the socket to the target address if it is not a listening socket
        if(!isListening && connect(socketFileDescriptor, rp -> ai_addr, rp -> ai_addrlen) == 1 &&  
        !wouldBlock()) {

            return -1; // failed to connect

        }
        // setting socket as listening 
        if(isListening && setsockopt(socketFileDescriptor, SOL_SOCKET, SO_REUSEADDR, 
            reinterpret_cast<const char*>(&one), sizeof(one) == -1)) {

                return -1;

        }

        if(isListening && bind(socketFileDescriptor, rp -> ai_addr, rp -> ai_addrlen) == -1) {

            return -1;
        }

        if(!isUdp && isListening && listen(socketFileDescriptor, maxTcpBacklog) == -1) {

            return -1;
        }

        if(isUdp && ttl) {

            const bool isMulticast = atoi(ip.c_str()) & 0xF0 == 0xE0;

            if(isMulticast && setMcastTTL(socketFileDescriptor, ttl)) {

                return -1;
            }

            if(!isMulticast && setTTL(socketFileDescriptor, ttl)) {

                return -1;

            }

        }

        if (needsTimestamp && !setTimestamp(socketFileDescriptor)) {

            return -1;

        }
    }

    if (result) {

        freeaddrinfo(result);

    }

    return socketFileDescriptor;

}

ssize_t Sockets::udpSend(int fileDescriptor, 
        char* data, 
        size_t size, 
        int port,
        const std::string &ip) {

    sockaddr_in destination{};

    // using IPv4
    destination.sin_family = AF_INET;

    // setting port
    // htons converts int to appropriate byte format
    destination.sin_port   = htons(port);

    // converting ip string to byte sequence
    if(inet_pton(AF_INET, ip.c_str(), &destination.sin_addr) != 1)
        return -1; 


    ssize_t send = sendto(fileDescriptor, 
        data, 
        size, 
        0, 
        reinterpret_cast<sockaddr*>(&destination),
        sizeof(destination));

    if(send == -1)
        perror("sendto");

    return send;

}