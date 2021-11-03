cmake_minimum_required(VERSION 3.10)
project(HalfwayMedia)
set(CMAKE_CXX_STANDARD 17)

include_directories("/usr/local/include/")
link_directories("/usr/local/lib/")

aux_source_directory(. SRCS)
add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} avcodec avdevice avfilter avutil avformat swscale swresample pthread)