cmake_minimum_required(VERSION 3.9)

project(perf)

add_library(perf Perf.cpp)

target_link_libraries(perf mb-ascii pthread jparser logger)