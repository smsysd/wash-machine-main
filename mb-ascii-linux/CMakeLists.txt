cmake_minimum_required(VERSION 3.9)

project(mb-ascii)

add_library(mb-ascii MbAsciiMaster.cpp)

target_link_libraries(mb-ascii crc wiringPi)