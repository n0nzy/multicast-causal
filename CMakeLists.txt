cmake_minimum_required(VERSION 3.8)
project(multicast_causal)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")
set(SOURCE_FILES main.cpp)

add_executable(multicast_causal ${SOURCE_FILES})