# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.18

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:


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

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/bin/cmake --regenerate-during-build -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache

.PHONY : rebuild_cache/fast

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "No interactive CMake dialog available..."
	/usr/bin/cmake -E echo No\ interactive\ CMake\ dialog\ available.
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache

.PHONY : edit_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/elpi/workspace/sm-sys/wash-machine-main/CMakeFiles /home/elpi/workspace/sm-sys/wash-machine-main//CMakeFiles/progress.marks
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/elpi/workspace/sm-sys/wash-machine-main/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean

.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named wash-machine

# Build rule for target.
wash-machine: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 wash-machine
.PHONY : wash-machine

# fast build rule for target.
wash-machine/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/wash-machine.dir/build.make CMakeFiles/wash-machine.dir/build
.PHONY : wash-machine/fast

#=============================================================================
# Target rules for targets named crc

# Build rule for target.
crc: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 crc
.PHONY : crc

# fast build rule for target.
crc/fast:
	$(MAKE) $(MAKESILENT) -f crc/CMakeFiles/crc.dir/build.make crc/CMakeFiles/crc.dir/build
.PHONY : crc/fast

#=============================================================================
# Target rules for targets named rgb332

# Build rule for target.
rgb332: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 rgb332
.PHONY : rgb332

# fast build rule for target.
rgb332/fast:
	$(MAKE) $(MAKESILENT) -f rgb332/CMakeFiles/rgb332.dir/build.make rgb332/CMakeFiles/rgb332.dir/build
.PHONY : rgb332/fast

#=============================================================================
# Target rules for targets named general-tools

# Build rule for target.
general-tools: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 general-tools
.PHONY : general-tools

# fast build rule for target.
general-tools/fast:
	$(MAKE) $(MAKESILENT) -f general-tools/CMakeFiles/general-tools.dir/build.make general-tools/CMakeFiles/general-tools.dir/build
.PHONY : general-tools/fast

#=============================================================================
# Target rules for targets named jparser

# Build rule for target.
jparser: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 jparser
.PHONY : jparser

# fast build rule for target.
jparser/fast:
	$(MAKE) $(MAKESILENT) -f jparser-linux/CMakeFiles/jparser.dir/build.make jparser-linux/CMakeFiles/jparser.dir/build
.PHONY : jparser/fast

#=============================================================================
# Target rules for targets named logger

# Build rule for target.
logger: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 logger
.PHONY : logger

# fast build rule for target.
logger/fast:
	$(MAKE) $(MAKESILENT) -f logger-linux/CMakeFiles/logger.dir/build.make logger-linux/CMakeFiles/logger.dir/build
.PHONY : logger/fast

#=============================================================================
# Target rules for targets named font

# Build rule for target.
font: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 font
.PHONY : font

# fast build rule for target.
font/fast:
	$(MAKE) $(MAKESILENT) -f font-linux/CMakeFiles/font.dir/build.make font-linux/CMakeFiles/font.dir/build
.PHONY : font/fast

#=============================================================================
# Target rules for targets named mspi

# Build rule for target.
mspi: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 mspi
.PHONY : mspi

# fast build rule for target.
mspi/fast:
	$(MAKE) $(MAKESILENT) -f mspi-linux/CMakeFiles/mspi.dir/build.make mspi-linux/CMakeFiles/mspi.dir/build
.PHONY : mspi/fast

#=============================================================================
# Target rules for targets named extboard

# Build rule for target.
extboard: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 extboard
.PHONY : extboard

# fast build rule for target.
extboard/fast:
	$(MAKE) $(MAKESILENT) -f extboard/CMakeFiles/extboard.dir/build.make extboard/CMakeFiles/extboard.dir/build
.PHONY : extboard/fast

#=============================================================================
# Target rules for targets named qrscaner

# Build rule for target.
qrscaner: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 qrscaner
.PHONY : qrscaner

# fast build rule for target.
qrscaner/fast:
	$(MAKE) $(MAKESILENT) -f qrscaner-linux/CMakeFiles/qrscaner.dir/build.make qrscaner-linux/CMakeFiles/qrscaner.dir/build
.PHONY : qrscaner/fast

#=============================================================================
# Target rules for targets named timer

# Build rule for target.
timer: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 timer
.PHONY : timer

# fast build rule for target.
timer/fast:
	$(MAKE) $(MAKESILENT) -f timer/CMakeFiles/timer.dir/build.make timer/CMakeFiles/timer.dir/build
.PHONY : timer/fast

#=============================================================================
# Target rules for targets named ledmatrix

# Build rule for target.
ledmatrix: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 ledmatrix
.PHONY : ledmatrix

# fast build rule for target.
ledmatrix/fast:
	$(MAKE) $(MAKESILENT) -f ledmatrix-linux/CMakeFiles/ledmatrix.dir/build.make ledmatrix-linux/CMakeFiles/ledmatrix.dir/build
.PHONY : ledmatrix/fast

#=============================================================================
# Target rules for targets named mb-ascii

# Build rule for target.
mb-ascii: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 mb-ascii
.PHONY : mb-ascii

# fast build rule for target.
mb-ascii/fast:
	$(MAKE) $(MAKESILENT) -f mb-ascii-linux/CMakeFiles/mb-ascii.dir/build.make mb-ascii-linux/CMakeFiles/mb-ascii.dir/build
.PHONY : mb-ascii/fast

#=============================================================================
# Target rules for targets named utils

# Build rule for target.
utils: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 utils
.PHONY : utils

# fast build rule for target.
utils/fast:
	$(MAKE) $(MAKESILENT) -f utils/CMakeFiles/utils.dir/build.make utils/CMakeFiles/utils.dir/build
.PHONY : utils/fast

#=============================================================================
# Target rules for targets named stdsiga

# Build rule for target.
stdsiga: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 stdsiga
.PHONY : stdsiga

# fast build rule for target.
stdsiga/fast:
	$(MAKE) $(MAKESILENT) -f linux-stdsiga/CMakeFiles/stdsiga.dir/build.make linux-stdsiga/CMakeFiles/stdsiga.dir/build
.PHONY : stdsiga/fast

main.o: main.cpp.o

.PHONY : main.o

# target to build an object file
main.cpp.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/wash-machine.dir/build.make CMakeFiles/wash-machine.dir/main.cpp.o
.PHONY : main.cpp.o

main.i: main.cpp.i

.PHONY : main.i

# target to preprocess a source file
main.cpp.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/wash-machine.dir/build.make CMakeFiles/wash-machine.dir/main.cpp.i
.PHONY : main.cpp.i

main.s: main.cpp.s

.PHONY : main.s

# target to generate assembly for a file
main.cpp.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/wash-machine.dir/build.make CMakeFiles/wash-machine.dir/main.cpp.s
.PHONY : main.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... edit_cache"
	@echo "... rebuild_cache"
	@echo "... crc"
	@echo "... extboard"
	@echo "... font"
	@echo "... general-tools"
	@echo "... jparser"
	@echo "... ledmatrix"
	@echo "... logger"
	@echo "... mb-ascii"
	@echo "... mspi"
	@echo "... qrscaner"
	@echo "... rgb332"
	@echo "... stdsiga"
	@echo "... timer"
	@echo "... utils"
	@echo "... wash-machine"
	@echo "... main.o"
	@echo "... main.i"
	@echo "... main.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

