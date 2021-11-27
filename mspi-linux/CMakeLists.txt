cmake_minimum_required(VERSION 3.9)

project(mspi)

add_library(mspi Mspi.cpp)

target_link_libraries(mspi crc pthread wiringPi general-tools)