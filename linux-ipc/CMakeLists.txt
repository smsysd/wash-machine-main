cmake_minimum_required(VERSION 3.9)
project(ipc)
add_library(ipc ipc.c)
target_link_libraries(ipc pthread)