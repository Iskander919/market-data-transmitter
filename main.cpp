#include <iostream>
#include <iomanip>
#include "sockets.h"
#include "md_protocol.h"

std::ostream &operator << (std::ostream &ss, std::array<uint8_t, 
    exchange::marketDataStructSize> &arr) {

        for (uint8_t i : arr) {

            ss << std::hex << std::setw(3) << (int)i << " ";
            
        }

        return ss;

}

int main() {

    /*
    exchange::marketDataTypeDef mds;
    mds.clientId = 1;
    mds.orderId = 1;
    mds.priceValue = 1;
    mds.priorityValue = 1;
    mds.quant = 1;
    mds.sideValue = exchange::side::SELL;
    mds.tickerId = 1;

    std::array<uint8_t, 43> arr = MarketDataProtocolUtils::combineByteSequence(mds);

    std::cout << arr << std::endl; 
    */

    std::cout << "Yo! Market data transmitter started" << std::endl;

    Sockets s;

    std::string ip = s.getInterfaceIp("lo");

    std::cout << "ip: " << ip << std::endl;

    return 0;
}