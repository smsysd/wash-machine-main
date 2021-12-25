# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.18

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/elpi/workspace/sm-sys/wash-machine-main

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/elpi/workspace/sm-sys/wash-machine-main

# Include any dependencies generated for this target.
include timer/CMakeFiles/timer.dir/depend.make

# Include the progress variables for this target.
include timer/CMakeFiles/timer.dir/progress.make

# Include the compile flags for this target's objects.
include timer/CMakeFiles/timer.dir/flags.make

timer/CMakeFiles/timer.dir/Timer.cpp.o: timer/CMakeFiles/timer.dir/flags.make
timer/CMakeFiles/timer.dir/Timer.cpp.o: timer/Timer.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/elpi/workspace/sm-sys/wash-machine-main/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object timer/CMakeFiles/timer.dir/Timer.cpp.o"
	cd /home/elpi/workspace/sm-sys/wash-machine-main/timer && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/timer.dir/Timer.cpp.o -c /home/elpi/workspace/sm-sys/wash-machine-main/timer/Timer.cpp

timer/CMakeFiles/timer.dir/Timer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/timer.dir/Timer.cpp.i"
	cd /home/elpi/workspace/sm-sys/wash-machine-main/timer && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/elpi/workspace/sm-sys/wash-machine-main/timer/Timer.cpp > CMakeFiles/timer.dir/Timer.cpp.i

timer/CMakeFiles/timer.dir/Timer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/timer.dir/Timer.cpp.s"
	cd /home/elpi/workspace/sm-sys/wash-machine-main/timer && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/elpi/workspace/sm-sys/wash-machine-main/timer/Timer.cpp -o CMakeFiles/timer.dir/Timer.cpp.s

# Object files for target timer
timer_OBJECTS = \
"CMakeFiles/timer.dir/Timer.cpp.o"

# External object files for target timer
timer_EXTERNAL_OBJECTS =

timer/libtimer.a: timer/CMakeFiles/timer.dir/Timer.cpp.o
timer/libtimer.a: timer/CMakeFiles/timer.dir/build.make
timer/libtimer.a: timer/CMakeFiles/timer.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/elpi/workspace/sm-sys/wash-machine-main/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libtimer.a"
	cd /home/elpi/workspace/sm-sys/wash-machine-main/timer && $(CMAKE_COMMAND) -P CMakeFiles/timer.dir/cmake_clean_target.cmake
	cd /home/elpi/workspace/sm-sys/wash-machine-main/timer && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/timer.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
timer/CMakeFiles/timer.dir/build: timer/libtimer.a

.PHONY : timer/CMakeFiles/timer.dir/build

timer/CMakeFiles/timer.dir/clean:
	cd /home/elpi/workspace/sm-sys/wash-machine-main/timer && $(CMAKE_COMMAND) -P CMakeFiles/timer.dir/cmake_clean.cmake
.PHONY : timer/CMakeFiles/timer.dir/clean

timer/CMakeFiles/timer.dir/depend:
	cd /home/elpi/workspace/sm-sys/wash-machine-main && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/elpi/workspace/sm-sys/wash-machine-main /home/elpi/workspace/sm-sys/wash-machine-main/timer /home/elpi/workspace/sm-sys/wash-machine-main /home/elpi/workspace/sm-sys/wash-machine-main/timer /home/elpi/workspace/sm-sys/wash-machine-main/timer/CMakeFiles/timer.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : timer/CMakeFiles/timer.dir/depend

