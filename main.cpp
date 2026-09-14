#include <iostream>
#include "sockets.h"

int main() {

    std::cout << "Yo! Market data transmitter started" << std::endl;

    Sockets s;

    std::string ip = s.getInterfaceIp("lo");

    std::cout << "ip: " << ip << std::endl;

    return 0;
}