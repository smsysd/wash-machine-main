cmake_minimum_required(VERSION 3.9)

project(rgb332)

add_library(rgb332 rgb332.c)

target_link_libraries(rgb332 m)