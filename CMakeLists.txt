cmake_minimum_required(VERSION 3.20)
project(proxy)

set(CMAKE_CXX_STANDARD 14)

add_executable(proxy main.cpp Server.h)
