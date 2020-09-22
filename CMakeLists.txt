cmake_minimum_required(VERSION 3.15)
project(HalfwayLive)

set(CMAKE_CXX_STANDARD 14)

link_libraries(avcodec avdevice avfilter avformat avutil swscale swresample)
add_executable(HalfwayLive main.cpp MediaIn.cpp MediaIn.h MediaFileIn.cpp MediaFileIn.h MediaOut.cpp MediaOut.h MediaFileOut.cpp MediaFileOut.h LiveStreamOut.cpp LiveStreamOut.h Utils.cpp Utils.h)