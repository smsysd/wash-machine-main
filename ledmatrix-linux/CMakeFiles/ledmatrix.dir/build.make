# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.13

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/jodzik/projects/finish/wash-machine-main

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/jodzik/projects/finish/wash-machine-main

# Include any dependencies generated for this target.
include ledmatrix-linux/CMakeFiles/ledmatrix.dir/depend.make

# Include the progress variables for this target.
include ledmatrix-linux/CMakeFiles/ledmatrix.dir/progress.make

# Include the compile flags for this target's objects.
include ledmatrix-linux/CMakeFiles/ledmatrix.dir/flags.make

ledmatrix-linux/CMakeFiles/ledmatrix.dir/LedMatrix.cpp.o: ledmatrix-linux/CMakeFiles/ledmatrix.dir/flags.make
ledmatrix-linux/CMakeFiles/ledmatrix.dir/LedMatrix.cpp.o: ledmatrix-linux/LedMatrix.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jodzik/projects/finish/wash-machine-main/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object ledmatrix-linux/CMakeFiles/ledmatrix.dir/LedMatrix.cpp.o"
	cd /home/jodzik/projects/finish/wash-machine-main/ledmatrix-linux && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ledmatrix.dir/LedMatrix.cpp.o -c /home/jodzik/projects/finish/wash-machine-main/ledmatrix-linux/LedMatrix.cpp

ledmatrix-linux/CMakeFiles/ledmatrix.dir/LedMatrix.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ledmatrix.dir/LedMatrix.cpp.i"
	cd /home/jodzik/projects/finish/wash-machine-main/ledmatrix-linux && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/jodzik/projects/finish/wash-machine-main/ledmatrix-linux/LedMatrix.cpp > CMakeFiles/ledmatrix.dir/LedMatrix.cpp.i

ledmatrix-linux/CMakeFiles/ledmatrix.dir/LedMatrix.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ledmatrix.dir/LedMatrix.cpp.s"
	cd /home/jodzik/projects/finish/wash-machine-main/ledmatrix-linux && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/jodzik/projects/finish/wash-machine-main/ledmatrix-linux/LedMatrix.cpp -o CMakeFiles/ledmatrix.dir/LedMatrix.cpp.s

# Object files for target ledmatrix
ledmatrix_OBJECTS = \
"CMakeFiles/ledmatrix.dir/LedMatrix.cpp.o"

# External object files for target ledmatrix
ledmatrix_EXTERNAL_OBJECTS =

ledmatrix-linux/libledmatrix.a: ledmatrix-linux/CMakeFiles/ledmatrix.dir/LedMatrix.cpp.o
ledmatrix-linux/libledmatrix.a: ledmatrix-linux/CMakeFiles/ledmatrix.dir/build.make
ledmatrix-linux/libledmatrix.a: ledmatrix-linux/CMakeFiles/ledmatrix.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/jodzik/projects/finish/wash-machine-main/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libledmatrix.a"
	cd /home/jodzik/projects/finish/wash-machine-main/ledmatrix-linux && $(CMAKE_COMMAND) -P CMakeFiles/ledmatrix.dir/cmake_clean_target.cmake
	cd /home/jodzik/projects/finish/wash-machine-main/ledmatrix-linux && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ledmatrix.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
ledmatrix-linux/CMakeFiles/ledmatrix.dir/build: ledmatrix-linux/libledmatrix.a

.PHONY : ledmatrix-linux/CMakeFiles/ledmatrix.dir/build

ledmatrix-linux/CMakeFiles/ledmatrix.dir/clean:
	cd /home/jodzik/projects/finish/wash-machine-main/ledmatrix-linux && $(CMAKE_COMMAND) -P CMakeFiles/ledmatrix.dir/cmake_clean.cmake
.PHONY : ledmatrix-linux/CMakeFiles/ledmatrix.dir/clean

ledmatrix-linux/CMakeFiles/ledmatrix.dir/depend:
	cd /home/jodzik/projects/finish/wash-machine-main && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/jodzik/projects/finish/wash-machine-main /home/jodzik/projects/finish/wash-machine-main/ledmatrix-linux /home/jodzik/projects/finish/wash-machine-main /home/jodzik/projects/finish/wash-machine-main/ledmatrix-linux /home/jodzik/projects/finish/wash-machine-main/ledmatrix-linux/CMakeFiles/ledmatrix.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : ledmatrix-linux/CMakeFiles/ledmatrix.dir/depend

