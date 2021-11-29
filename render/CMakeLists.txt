cmake_minimum_required(VERSION 3.9)

project(render)

add_library(render render.cpp)

target_link_libraries(render extboard rgb332)