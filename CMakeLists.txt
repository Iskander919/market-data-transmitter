cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)

project (market_data_transmitter)

add_executable(market_data_transmitter 
    # main.cpp
    examples/matching_engine.cpp
    sockets/sockets.cpp
    lock_free_queue/lock_free_queue.h
    market_data_publisher/md_protocol.cpp
    matching_engine/matching_engine.cpp
    order_gateway/order_gateway.cpp
    )

target_include_directories(market_data_transmitter PRIVATE 
examples/
sockets/
logger/
lock_free_queue/
matching_engine/
market_data_publisher/
order_gateway/)

