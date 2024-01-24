cmake_minimum_required(VERSION 3.10)
project(HalfwayMedia)
set(CMAKE_CXX_STANDARD 17)

set(NETWORK_SRCS
    ../tcp_client.cpp
    ../tcp_server.cpp
    ../udp_client.cpp
    ../udp_server.cpp
    ../event_processor.cpp
    ../event_reactor.cpp
    ../../common/thread_pool.cpp
    ../../common/data_buffer.cpp
    ../../common/utils.cpp)

# set(CMAKE_CXX_FLAGS "-DRELEASE")
add_library(network ${NETWORK_SRCS})

add_executable(tcp_client tcp_client.cxx)
target_link_libraries(tcp_client pthread network avutil)

add_executable(udp_client udp_client.cxx)
target_link_libraries(udp_client pthread network avutil)

add_executable(tcp_echo_server tcp_echo_server.cxx)
target_link_libraries(tcp_echo_server pthread network avutil)

add_executable(udp_echo_server udp_echo_server.cxx)
target_link_libraries(udp_echo_server avutil pthread network)
