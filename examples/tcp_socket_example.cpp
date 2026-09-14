
#include <iostream>
#include "logger.h"
#include "sockets.h"

int main() {

    std::string fileName = "udp_log.txt";

    Sockets socket;
    Logger logger(fileName);
    logger.startLoggerThread();

    std::string ip = "";

    // std::string interface is the name of the 
    // network interface we want to use
    // "lo" stands for localhost
    std::string interface = "lo";
    int port = 12345;

    logger.log("Entered main\n");

    int fd = socket.createSocket(ip, 
        interface, 
        port, 
        true,
        true,
        false,
        1,
        false,
        logger);

    char data[4] = {0x01, 0x02, 0x03, 0x32};

    while(1) {

        ssize_t sendStatus = socket.udpSend(fd, data, 4, 9000, "127.0.0.1");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "Sent" << std::endl;

    }

    return 0;
}