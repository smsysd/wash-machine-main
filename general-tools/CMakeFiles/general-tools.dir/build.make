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
include general-tools/CMakeFiles/general-tools.dir/depend.make

# Include the progress variables for this target.
include general-tools/CMakeFiles/general-tools.dir/progress.make

# Include the compile flags for this target's objects.
include general-tools/CMakeFiles/general-tools.dir/flags.make

general-tools/CMakeFiles/general-tools.dir/general_tools.c.o: general-tools/CMakeFiles/general-tools.dir/flags.make
general-tools/CMakeFiles/general-tools.dir/general_tools.c.o: general-tools/general_tools.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/elpi/workspace/sm-sys/wash-machine-main/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object general-tools/CMakeFiles/general-tools.dir/general_tools.c.o"
	cd /home/elpi/workspace/sm-sys/wash-machine-main/general-tools && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/general-tools.dir/general_tools.c.o -c /home/elpi/workspace/sm-sys/wash-machine-main/general-tools/general_tools.c

general-tools/CMakeFiles/general-tools.dir/general_tools.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/general-tools.dir/general_tools.c.i"
	cd /home/elpi/workspace/sm-sys/wash-machine-main/general-tools && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/elpi/workspace/sm-sys/wash-machine-main/general-tools/general_tools.c > CMakeFiles/general-tools.dir/general_tools.c.i

general-tools/CMakeFiles/general-tools.dir/general_tools.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/general-tools.dir/general_tools.c.s"
	cd /home/elpi/workspace/sm-sys/wash-machine-main/general-tools && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/elpi/workspace/sm-sys/wash-machine-main/general-tools/general_tools.c -o CMakeFiles/general-tools.dir/general_tools.c.s

# Object files for target general-tools
general__tools_OBJECTS = \
"CMakeFiles/general-tools.dir/general_tools.c.o"

# External object files for target general-tools
general__tools_EXTERNAL_OBJECTS =

general-tools/libgeneral-tools.a: general-tools/CMakeFiles/general-tools.dir/general_tools.c.o
general-tools/libgeneral-tools.a: general-tools/CMakeFiles/general-tools.dir/build.make
general-tools/libgeneral-tools.a: general-tools/CMakeFiles/general-tools.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/elpi/workspace/sm-sys/wash-machine-main/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C static library libgeneral-tools.a"
	cd /home/elpi/workspace/sm-sys/wash-machine-main/general-tools && $(CMAKE_COMMAND) -P CMakeFiles/general-tools.dir/cmake_clean_target.cmake
	cd /home/elpi/workspace/sm-sys/wash-machine-main/general-tools && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/general-tools.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
general-tools/CMakeFiles/general-tools.dir/build: general-tools/libgeneral-tools.a

.PHONY : general-tools/CMakeFiles/general-tools.dir/build

general-tools/CMakeFiles/general-tools.dir/clean:
	cd /home/elpi/workspace/sm-sys/wash-machine-main/general-tools && $(CMAKE_COMMAND) -P CMakeFiles/general-tools.dir/cmake_clean.cmake
.PHONY : general-tools/CMakeFiles/general-tools.dir/clean

general-tools/CMakeFiles/general-tools.dir/depend:
	cd /home/elpi/workspace/sm-sys/wash-machine-main && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/elpi/workspace/sm-sys/wash-machine-main /home/elpi/workspace/sm-sys/wash-machine-main/general-tools /home/elpi/workspace/sm-sys/wash-machine-main /home/elpi/workspace/sm-sys/wash-machine-main/general-tools /home/elpi/workspace/sm-sys/wash-machine-main/general-tools/CMakeFiles/general-tools.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : general-tools/CMakeFiles/general-tools.dir/depend

