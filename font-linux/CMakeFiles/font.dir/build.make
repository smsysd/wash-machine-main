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
include font-linux/CMakeFiles/font.dir/depend.make

# Include the progress variables for this target.
include font-linux/CMakeFiles/font.dir/progress.make

# Include the compile flags for this target's objects.
include font-linux/CMakeFiles/font.dir/flags.make

font-linux/CMakeFiles/font.dir/Font.cpp.o: font-linux/CMakeFiles/font.dir/flags.make
font-linux/CMakeFiles/font.dir/Font.cpp.o: font-linux/Font.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/elpi/workspace/sm-sys/wash-machine-main/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object font-linux/CMakeFiles/font.dir/Font.cpp.o"
	cd /home/elpi/workspace/sm-sys/wash-machine-main/font-linux && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/font.dir/Font.cpp.o -c /home/elpi/workspace/sm-sys/wash-machine-main/font-linux/Font.cpp

font-linux/CMakeFiles/font.dir/Font.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/font.dir/Font.cpp.i"
	cd /home/elpi/workspace/sm-sys/wash-machine-main/font-linux && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/elpi/workspace/sm-sys/wash-machine-main/font-linux/Font.cpp > CMakeFiles/font.dir/Font.cpp.i

font-linux/CMakeFiles/font.dir/Font.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/font.dir/Font.cpp.s"
	cd /home/elpi/workspace/sm-sys/wash-machine-main/font-linux && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/elpi/workspace/sm-sys/wash-machine-main/font-linux/Font.cpp -o CMakeFiles/font.dir/Font.cpp.s

# Object files for target font
font_OBJECTS = \
"CMakeFiles/font.dir/Font.cpp.o"

# External object files for target font
font_EXTERNAL_OBJECTS =

font-linux/libfont.a: font-linux/CMakeFiles/font.dir/Font.cpp.o
font-linux/libfont.a: font-linux/CMakeFiles/font.dir/build.make
font-linux/libfont.a: font-linux/CMakeFiles/font.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/elpi/workspace/sm-sys/wash-machine-main/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libfont.a"
	cd /home/elpi/workspace/sm-sys/wash-machine-main/font-linux && $(CMAKE_COMMAND) -P CMakeFiles/font.dir/cmake_clean_target.cmake
	cd /home/elpi/workspace/sm-sys/wash-machine-main/font-linux && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/font.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
font-linux/CMakeFiles/font.dir/build: font-linux/libfont.a

.PHONY : font-linux/CMakeFiles/font.dir/build

font-linux/CMakeFiles/font.dir/clean:
	cd /home/elpi/workspace/sm-sys/wash-machine-main/font-linux && $(CMAKE_COMMAND) -P CMakeFiles/font.dir/cmake_clean.cmake
.PHONY : font-linux/CMakeFiles/font.dir/clean

font-linux/CMakeFiles/font.dir/depend:
	cd /home/elpi/workspace/sm-sys/wash-machine-main && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/elpi/workspace/sm-sys/wash-machine-main /home/elpi/workspace/sm-sys/wash-machine-main/font-linux /home/elpi/workspace/sm-sys/wash-machine-main /home/elpi/workspace/sm-sys/wash-machine-main/font-linux /home/elpi/workspace/sm-sys/wash-machine-main/font-linux/CMakeFiles/font.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : font-linux/CMakeFiles/font.dir/depend
