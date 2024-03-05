cmake_minimum_required(VERSION 3.10)
project(HalfwayMedia)
set(CMAKE_CXX_STANDARD 17)

include_directories("/usr/local/include/" ../../../)

set(RTCP_SRCS ../rtcp.cpp ../../../common/log.cpp
              ../../../common/data_buffer.cpp)

# set(CMAKE_CXX_FLAGS "-DRELEASE")
add_executable(rtcp_test rtcp_test.cxx ${RTCP_SRCS})
