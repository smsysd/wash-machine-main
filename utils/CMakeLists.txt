cmake_minimum_required(VERSION 3.9)

project(utils)

add_library(utils utils.cpp)

target_link_libraries(utils extboard jparser logger general-tools qrscaner render)