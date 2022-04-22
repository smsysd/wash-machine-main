cmake_minimum_required(VERSION 3.9)

project(wash-machine-main)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY mbuild)

add_subdirectory(crc)
add_subdirectory(rgb332)
add_subdirectory(general-tools)
add_subdirectory(jparser-linux)
add_subdirectory(logger-linux)
add_subdirectory(font-linux)
add_subdirectory(mspi-linux)
add_subdirectory(extboard)
add_subdirectory(qrscaner-linux)
add_subdirectory(timer)
add_subdirectory(ledmatrix-linux)
add_subdirectory(mb-ascii-linux)
add_subdirectory(utils)
add_subdirectory(linux-stdsiga)
add_subdirectory(linux-ipc)

add_executable(wash-machine-main main.cpp)

target_link_libraries(wash-machine-main utils extboard general-tools logger jparser)