cmake_minimum_required(VERSION 3.9)

project(extboard)

add_library(extboard ExtBoard.cpp)

target_link_libraries(extboard ecbm pthread general-tools jparser logger)