cmake_minimum_required(VERSION 3.10)
project(HalfwayMedia)
set(CMAKE_CXX_STANDARD 17)

include_directories("/usr/local/include/" src)
link_directories("/usr/local/lib/")

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE EXCLUDE_FILES "CMakeCXXCompilerId.cpp")
list(REMOVE_ITEM SOURCES ${EXCLUDE_FILES})
message(STATUS ${SOURCES})

set(CMAKE_CXX_FLAGS "-Wno-error")
add_executable(${PROJECT_NAME} ${SOURCES})

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
