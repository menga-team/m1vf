cmake_minimum_required(VERSION 3.21)
project(m1play)

set(CMAKE_CXX_STANDARD 14)

include_directories(decoder)

add_executable(
        decoder/m1vf.h
        player/main.cpp)
