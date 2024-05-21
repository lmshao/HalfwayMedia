cmake_minimum_required(VERSION 3.10)
project(HalfwayMedia)
set(CMAKE_CXX_STANDARD 17)

include_directories("/usr/local/include/" ${PROJECT_SOURCE_DIR}
                    ${PROJECT_SOURCE_DIR}/network/include)
link_directories("/usr/local/lib/")

add_subdirectory(network)

file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE EXCLUDE_FILES "CMakeCXXCompilerId.cpp" "apps/*.cpp")
list(REMOVE_ITEM SOURCES ${EXCLUDE_FILES})

set(CMAKE_CXX_FLAGS "-Wno-error -g")
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE network)
# target_include_directories(${PROJECT_NAME} PRIVATE
# ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE APP_SOURCES "apps/*.cpp")
foreach(src IN LISTS APP_SOURCES)
  string(REGEX REPLACE "^.*/([^/.]+).cpp" "\\1" target ${src})
  message(STATUS "target: " ${target})
  link_libraries(${PROJECT_NAME} avcodec avformat)
  add_executable(${target} ${src})
endforeach(src)
