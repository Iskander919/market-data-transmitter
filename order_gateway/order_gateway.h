#include "md_protocol.h"

namespace exchange {

    enum class clientResponseTypeEnum : uint8_t {

        INVALID         = 0,
        ACCEPTED        = 1,
        CANCELED        = 2,
        FILLED          = 3,
        CANCEL_REJECTED = 4

    };

    // (MEClientResponse according to the book)
    // this struct contains data that exchange transfers to clients 
    // in response to their orders
    struct clientResponseTypeDef {

        explicit clientResponseTypeDef();

        clientResponseTypeEnum responseStatus;
        client_id clientId;
        ticker_id tickerId;
        order_id  marketOrderId;
        order_id  clientOrderId;
        side      sideValue;
        price     priceValue;
        quantity  qty;
        priority  prty;

        std::string toString();

    };

    // (MEClientRequest according to the book)
    // this struct represents the requests that clients 
    // send to the exchange server 
    struct clientRequestTypeDef {

        explicit clientRequestTypeDef();

        client_id clientId;
        ticker_id tickerId;
        order_id  orderId;
        side      sideValue;
        price     priceValue;
        quantity  qty;

        std::string toString();

    };

}