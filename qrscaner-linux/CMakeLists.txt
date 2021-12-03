cmake_minimum_required(VERSION 3.9)

project(qrscaner)

add_library(qrscaner qrscaner.c)

target_link_libraries(qrscaner pthread)