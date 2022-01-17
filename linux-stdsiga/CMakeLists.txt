cmake_minimum_required(VERSION 3.9)

project(stdsiga)

add_library(stdsiga stdsiga.c)

target_link_libraries(stdsiga pthread)