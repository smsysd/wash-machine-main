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
include logger-linux/CMakeFiles/logger.dir/depend.make

# Include the progress variables for this target.
include logger-linux/CMakeFiles/logger.dir/progress.make

# Include the compile flags for this target's objects.
include logger-linux/CMakeFiles/logger.dir/flags.make

logger-linux/CMakeFiles/logger.dir/Logger.cpp.o: logger-linux/CMakeFiles/logger.dir/flags.make
logger-linux/CMakeFiles/logger.dir/Logger.cpp.o: logger-linux/Logger.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jodzik/projects/finish/wash-machine-main/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object logger-linux/CMakeFiles/logger.dir/Logger.cpp.o"
	cd /home/jodzik/projects/finish/wash-machine-main/logger-linux && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/logger.dir/Logger.cpp.o -c /home/jodzik/projects/finish/wash-machine-main/logger-linux/Logger.cpp

logger-linux/CMakeFiles/logger.dir/Logger.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/logger.dir/Logger.cpp.i"
	cd /home/jodzik/projects/finish/wash-machine-main/logger-linux && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/jodzik/projects/finish/wash-machine-main/logger-linux/Logger.cpp > CMakeFiles/logger.dir/Logger.cpp.i

logger-linux/CMakeFiles/logger.dir/Logger.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/logger.dir/Logger.cpp.s"
	cd /home/jodzik/projects/finish/wash-machine-main/logger-linux && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/jodzik/projects/finish/wash-machine-main/logger-linux/Logger.cpp -o CMakeFiles/logger.dir/Logger.cpp.s

# Object files for target logger
logger_OBJECTS = \
"CMakeFiles/logger.dir/Logger.cpp.o"

# External object files for target logger
logger_EXTERNAL_OBJECTS =

logger-linux/liblogger.a: logger-linux/CMakeFiles/logger.dir/Logger.cpp.o
logger-linux/liblogger.a: logger-linux/CMakeFiles/logger.dir/build.make
logger-linux/liblogger.a: logger-linux/CMakeFiles/logger.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/jodzik/projects/finish/wash-machine-main/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library liblogger.a"
	cd /home/jodzik/projects/finish/wash-machine-main/logger-linux && $(CMAKE_COMMAND) -P CMakeFiles/logger.dir/cmake_clean_target.cmake
	cd /home/jodzik/projects/finish/wash-machine-main/logger-linux && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/logger.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
logger-linux/CMakeFiles/logger.dir/build: logger-linux/liblogger.a

.PHONY : logger-linux/CMakeFiles/logger.dir/build

logger-linux/CMakeFiles/logger.dir/clean:
	cd /home/jodzik/projects/finish/wash-machine-main/logger-linux && $(CMAKE_COMMAND) -P CMakeFiles/logger.dir/cmake_clean.cmake
.PHONY : logger-linux/CMakeFiles/logger.dir/clean

logger-linux/CMakeFiles/logger.dir/depend:
	cd /home/jodzik/projects/finish/wash-machine-main && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/jodzik/projects/finish/wash-machine-main /home/jodzik/projects/finish/wash-machine-main/logger-linux /home/jodzik/projects/finish/wash-machine-main /home/jodzik/projects/finish/wash-machine-main/logger-linux /home/jodzik/projects/finish/wash-machine-main/logger-linux/CMakeFiles/logger.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : logger-linux/CMakeFiles/logger.dir/depend

