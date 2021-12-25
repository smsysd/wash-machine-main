cmake_minimum_required(VERSION 3.9)

project(font)

add_library(font Font.cpp)

target_link_libraries(font rgb332)