cmake_minimum_required(VERSION 3.9)

project(timer)

add_library(timer Timer.cpp)

target_link_libraries(timer rt)