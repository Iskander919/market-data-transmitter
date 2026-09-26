#include "order_gateway.h"

exchange::clientResponseTypeDef::clientResponseTypeDef() :  
    clientId(CLIENT_ID_INVALID), tickerId(TICKER_ID_INVALID), marketOrderId(ORDER_ID_INVALID), 
    clientOrderId(ORDER_ID_INVALID), sideValue(side::INVALID), priceValue(PRICE_INVALID), 
    qty(QUANTITY_INVALID), prty(PRIORITY_INVALID) { }

std::string exchange::clientResponseTypeDef::toString() {

    return "";

}

exchange::clientRequestTypeDef::clientRequestTypeDef() : 
    clientId(CLIENT_ID_INVALID), tickerId(TICKER_ID_INVALID), orderId(ORDER_ID_INVALID),
    sideValue(side::INVALID), priceValue(PRICE_INVALID), qty(QUANTITY_INVALID) { }

std::string exchange::clientRequestTypeDef::toString() {

    return "";

}