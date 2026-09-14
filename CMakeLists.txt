cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)

project (market_data_transmitter)

add_executable(market_data_transmitter 
    # main.cpp
    examples/tcp_socket_example.cpp
    sockets/sockets.cpp
    lock_free_queue/lock_free_queue.h
    md_protocol/md_protocol.cpp)

target_include_directories(market_data_transmitter PRIVATE 
examples/
sockets/
logger/
lock_free_queue/
md_protocol/)
