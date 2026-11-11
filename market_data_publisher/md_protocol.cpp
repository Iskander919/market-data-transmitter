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

    // converting market update type
    combined[1] = std::bit_cast<uint8_t>(mdStruct.marketUpdateType);

    // converting order_id
    temp8 = std::bit_cast<std::array<uint8_t, 8>>(mdStruct.orderId);
    std::copy(temp8.begin(), temp8.end(), combined.begin() + 2);     // bytes 2..9

    // converting ticker_id
    temp4 = std::bit_cast<std::array<uint8_t, 4>>(mdStruct.tickerId);
    std::copy(temp4.begin(), temp4.end(), combined.begin() + 10);    // 10..13

    // converting price
    temp8 = std::bit_cast<std::array<uint8_t, 8>>(mdStruct.priceValue);
    std::copy(temp8.begin(), temp8.end(), combined.begin() + 14);    // 14..21

    // converting quantity
    temp8 = std::bit_cast<std::array<uint8_t, 8>>(mdStruct.quant);
    std::copy(temp8.begin(), temp8.end(), combined.begin() + 22);    // 22..29

    // converting priority
    temp8 = std::bit_cast<std::array<uint8_t, 8>>(mdStruct.priorityValue);
    std::copy(temp8.begin(), temp8.end(), combined.begin() + 30);    // 30..37

    // converting side
    combined[38] = std::bit_cast<uint8_t>(mdStruct.sideValue);

    // end of frame
    combined[39] = 0xFF;

    return combined;

}