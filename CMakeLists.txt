cmake_minimum_required(VERSION 3.9)
project(schoenberg)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

include_directories(src)

add_subdirectory(src)
add_subdirectory(tst)

