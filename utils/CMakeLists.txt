cmake_minimum_required(VERSION 3.9)

project(utils)

add_library(utils utils.cpp bonus.cpp button_driver.cpp render.cpp)

target_link_libraries(utils extboard perf jparser logger general-tools qrscaner timer pthread stdsiga ipc)