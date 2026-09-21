#include "md_protocol.h"

/**
 * @brief
 * @param
 * @return 
 */
std::array<uint8_t, exchange::marketDataStructSize> 
MarketDataProtocolUtils::combineByteSequence(const exchange::marketDataTypeDef &mdStruct) {

    // converting mdStruct fields into bytes
    std::array <uint8_t, 8> temp8 = {};
    std::array <uint8_t, 4> temp4 = {};
    std::array <uint8_t, exchange::marketDataStructSize> combined = {};

    combined[0] = 0xAA; // start of frame

    // converting order_id
    temp8 = std::bit_cast<std::array<uint8_t, 8>>(mdStruct.orderId);
    std::copy(combined.begin() + 1, combined.begin() + 8, temp8.begin());

    // converting ticker_id
    temp4 = std::bit_cast<std::array<uint8_t, 4>>(mdStruct.tickerId);
    std::copy(combined.begin() + 9, combined.begin() + 12, temp4.begin());

    // converting client_id
    temp4 = std::bit_cast<std::array<uint8_t, 4>>(mdStruct.clientId);
    std::copy(combined.begin() + 13, combined.begin() + 16, temp4.begin());

    // converting price
    temp8 = std::bit_cast<std::array<uint8_t, 8>>(mdStruct.priceValue);
    std::copy(combined.begin() + 17, combined.begin() + 24, temp8.begin());

    // converting quantity
    temp8 = std::bit_cast<std::array<uint8_t, 8>>(mdStruct.quant);
    std::copy(combined.begin() + 25, combined.begin() + 32, temp8.begin());

    // converting priority
    temp8 = std::bit_cast<std::array<uint8_t, 8>>(mdStruct.priorityValue);
    std::copy(combined.begin() + 33, combined.begin() + 40, temp8.begin());

    // converting side
    combined[41] = std::bit_cast<uint8_t>(mdStruct.sideValue);

    // end of frame
    combined[42] = 0xFF;

    return combined;

}