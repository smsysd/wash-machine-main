cmake_minimum_required(VERSION 3.9)

project(general-tools)

add_library(general-tools general_tools.c)

target_link_libraries(general-tools pthread)