cmake_minimum_required(VERSION 3.10)
project(HalfwayLive)

set(CMAKE_CXX_STANDARD 14)
#include_directories(SYSTEM "/usr/local/include/")
#link_directories(SYSTEM "/usr/local/lib/")
link_libraries(avcodec avdevice avfilter avformat avutil swscale swresample)
add_executable(HalfwayLive main.cpp MediaIn.cpp MediaIn.h MediaFileIn.cpp MediaFileIn.h MediaOut.cpp MediaOut.h MediaFileOut.cpp MediaFileOut.h LiveStreamOut.cpp LiveStreamOut.h Utils.cpp Utils.h)