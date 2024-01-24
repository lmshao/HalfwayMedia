cmake_minimum_required(VERSION 3.10)
project(HalfwayMedia)
set(CMAKE_CXX_STANDARD 17)

include_directories("/usr/local/include/" src)
link_directories("/usr/local/lib/")

set(CMAKE_CXX_FLAGS "-Wno-error")

file(REMOVE_RECURSE src/network/test/build)

file(GLOB_RECURSE SRCS "src/*.cpp")
message(STATUS "${SRCS}")
add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(
  ${PROJECT_NAME}
  avcodec
  avdevice
  avfilter
  avutil
  avformat
  swscale
  swresample
  pthread)
