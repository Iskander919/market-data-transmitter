cmake_minimum_required(VERSION 3.16)

project(market_data_transmitter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)

add_library(market_data_core
    market_data_publisher/md_protocol.cpp
    matching_engine/matching_engine.cpp
    order_gateway/order_gateway.cpp
    limit_order_book/limit_order_book.cpp
    sockets/sockets.cpp
)

target_include_directories(market_data_core PUBLIC
    logger/
    lock_free_queue/
    matching_engine/
    market_data_publisher/
    order_gateway/
    mem_pool/
    limit_order_book/
    sockets/
)

add_executable(market_data_transmitter
    examples/limit_order_book.cpp
    sockets/sockets.cpp
)

target_include_directories(market_data_transmitter PRIVATE
    examples/
    sockets/
)

target_link_libraries(market_data_transmitter
    PRIVATE market_data_core
)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)

    add_executable(limit_order_book_tests
        tests/limit_order_book_tests.cpp
    )

    target_link_libraries(limit_order_book_tests
        PRIVATE
            market_data_core
            GTest::gtest_main
    )

    add_test(
        NAME limit_order_book_tests
        COMMAND limit_order_book_tests
    )
endif()
