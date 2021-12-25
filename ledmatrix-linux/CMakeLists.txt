cmake_minimum_required(VERSION 3.9)

project(ledmatrix)

add_library(ledmatrix LedMatrix.cpp)

target_link_libraries(ledmatrix crc font mb-ascii)
