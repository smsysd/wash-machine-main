cmake_minimum_required(VERSION 3.9)

project(wash-machine-main)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY mbuild)

add_subdirectory(crc)
add_subdirectory(general-tools)
add_subdirectory(jparser-linux)
add_subdirectory(logger-linux)
add_subdirectory(extboard)
add_subdirectory(perf)
add_subdirectory(qrscaner-linux)
add_subdirectory(timer)
add_subdirectory(mb-ascii-linux)
add_subdirectory(utils)
add_subdirectory(linux-stdsiga)
add_subdirectory(linux-ipc)
add_subdirectory(linux-ecbm)

add_executable(wash-machine-main main.cpp)

target_link_libraries(wash-machine-main utils extboard general-tools logger jparser)