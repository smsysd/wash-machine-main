cmake_minimum_required(VERSION 3.9)

project(crc)

add_library(crc crc8.c lrc8.c crc16.c crc32.c)