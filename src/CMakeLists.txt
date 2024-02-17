cmake_minimum_required(VERSION 3.10)
project(HalfwayMedia)
set(CMAKE_CXX_STANDARD 17)

include_directories("/usr/local/include/" .)
link_directories("/usr/local/lib/")

file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE EXCLUDE_FILES "CMakeCXXCompilerId.cpp" "apps/*.cpp")
list(REMOVE_ITEM SOURCES ${EXCLUDE_FILES})

set(CMAKE_CXX_FLAGS "-Wno-error -g")
add_library(${PROJECT_NAME} SHARED ${SOURCES})

file(GLOB_RECURSE APP_SOURCES "apps/*.cpp")
foreach(src IN LISTS APP_SOURCES)
  string(REGEX REPLACE "^.*/([^/.]+).cpp" "\\1" target ${src})
  message(STATUS "target: " ${target})
  link_libraries(${PROJECT_NAME} avcodec avformat)
  add_executable(${target} ${src})
endforeach(src)
