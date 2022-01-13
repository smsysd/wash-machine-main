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
CMAKE_SOURCE_DIR = /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux

# Include any dependencies generated for this target.
include rgb332/CMakeFiles/rgb332.dir/depend.make

# Include the progress variables for this target.
include rgb332/CMakeFiles/rgb332.dir/progress.make

# Include the compile flags for this target's objects.
include rgb332/CMakeFiles/rgb332.dir/flags.make

rgb332/CMakeFiles/rgb332.dir/rgb332.c.o: rgb332/CMakeFiles/rgb332.dir/flags.make
rgb332/CMakeFiles/rgb332.dir/rgb332.c.o: rgb332/rgb332.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object rgb332/CMakeFiles/rgb332.dir/rgb332.c.o"
	cd /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux/rgb332 && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/rgb332.dir/rgb332.c.o -c /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux/rgb332/rgb332.c

rgb332/CMakeFiles/rgb332.dir/rgb332.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/rgb332.dir/rgb332.c.i"
	cd /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux/rgb332 && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux/rgb332/rgb332.c > CMakeFiles/rgb332.dir/rgb332.c.i

rgb332/CMakeFiles/rgb332.dir/rgb332.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/rgb332.dir/rgb332.c.s"
	cd /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux/rgb332 && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux/rgb332/rgb332.c -o CMakeFiles/rgb332.dir/rgb332.c.s

# Object files for target rgb332
rgb332_OBJECTS = \
"CMakeFiles/rgb332.dir/rgb332.c.o"

# External object files for target rgb332
rgb332_EXTERNAL_OBJECTS =

rgb332/librgb332.a: rgb332/CMakeFiles/rgb332.dir/rgb332.c.o
rgb332/librgb332.a: rgb332/CMakeFiles/rgb332.dir/build.make
rgb332/librgb332.a: rgb332/CMakeFiles/rgb332.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C static library librgb332.a"
	cd /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux/rgb332 && $(CMAKE_COMMAND) -P CMakeFiles/rgb332.dir/cmake_clean_target.cmake
	cd /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux/rgb332 && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/rgb332.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
rgb332/CMakeFiles/rgb332.dir/build: rgb332/librgb332.a

.PHONY : rgb332/CMakeFiles/rgb332.dir/build

rgb332/CMakeFiles/rgb332.dir/clean:
	cd /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux/rgb332 && $(CMAKE_COMMAND) -P CMakeFiles/rgb332.dir/cmake_clean.cmake
.PHONY : rgb332/CMakeFiles/rgb332.dir/clean

rgb332/CMakeFiles/rgb332.dir/depend:
	cd /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux/rgb332 /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux/rgb332 /home/elpi/workspace/sm-sys-gen/ledmatrix/ledmatrix-linux/rgb332/CMakeFiles/rgb332.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : rgb332/CMakeFiles/rgb332.dir/depend

