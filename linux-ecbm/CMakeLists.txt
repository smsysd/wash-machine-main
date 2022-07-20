cmake_minimum_required(VERSION 3.9)

project(ecbm)

add_library(ecbm ecbm.c ecbm_core.c)

target_link_libraries(ecbm crc pthread)