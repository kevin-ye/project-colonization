# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.4

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
CMAKE_COMMAND = /opt/local/bin/cmake

# The command to remove a file.
RM = /opt/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1

# Include any dependencies generated for this target.
include examples/CMakeFiles/Boing.dir/depend.make

# Include the progress variables for this target.
include examples/CMakeFiles/Boing.dir/progress.make

# Include the compile flags for this target's objects.
include examples/CMakeFiles/Boing.dir/flags.make

examples/CMakeFiles/Boing.dir/boing.c.o: examples/CMakeFiles/Boing.dir/flags.make
examples/CMakeFiles/Boing.dir/boing.c.o: examples/boing.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object examples/CMakeFiles/Boing.dir/boing.c.o"
	cd /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1/examples && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Boing.dir/boing.c.o   -c /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1/examples/boing.c

examples/CMakeFiles/Boing.dir/boing.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Boing.dir/boing.c.i"
	cd /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1/examples && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1/examples/boing.c > CMakeFiles/Boing.dir/boing.c.i

examples/CMakeFiles/Boing.dir/boing.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Boing.dir/boing.c.s"
	cd /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1/examples && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1/examples/boing.c -o CMakeFiles/Boing.dir/boing.c.s

examples/CMakeFiles/Boing.dir/boing.c.o.requires:

.PHONY : examples/CMakeFiles/Boing.dir/boing.c.o.requires

examples/CMakeFiles/Boing.dir/boing.c.o.provides: examples/CMakeFiles/Boing.dir/boing.c.o.requires
	$(MAKE) -f examples/CMakeFiles/Boing.dir/build.make examples/CMakeFiles/Boing.dir/boing.c.o.provides.build
.PHONY : examples/CMakeFiles/Boing.dir/boing.c.o.provides

examples/CMakeFiles/Boing.dir/boing.c.o.provides.build: examples/CMakeFiles/Boing.dir/boing.c.o


# Object files for target Boing
Boing_OBJECTS = \
"CMakeFiles/Boing.dir/boing.c.o"

# External object files for target Boing
Boing_EXTERNAL_OBJECTS =

examples/Boing.app/Contents/MacOS/Boing: examples/CMakeFiles/Boing.dir/boing.c.o
examples/Boing.app/Contents/MacOS/Boing: examples/CMakeFiles/Boing.dir/build.make
examples/Boing.app/Contents/MacOS/Boing: src/libglfw3.a
examples/Boing.app/Contents/MacOS/Boing: examples/CMakeFiles/Boing.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable Boing.app/Contents/MacOS/Boing"
	cd /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1/examples && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Boing.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
examples/CMakeFiles/Boing.dir/build: examples/Boing.app/Contents/MacOS/Boing

.PHONY : examples/CMakeFiles/Boing.dir/build

examples/CMakeFiles/Boing.dir/requires: examples/CMakeFiles/Boing.dir/boing.c.o.requires

.PHONY : examples/CMakeFiles/Boing.dir/requires

examples/CMakeFiles/Boing.dir/clean:
	cd /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1/examples && $(CMAKE_COMMAND) -P CMakeFiles/Boing.dir/cmake_clean.cmake
.PHONY : examples/CMakeFiles/Boing.dir/clean

examples/CMakeFiles/Boing.dir/depend:
	cd /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1 /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1/examples /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1 /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1/examples /Users/Ye-Macbook/Documents/Development/cs488/project/shared/glfw-3.1.1/examples/CMakeFiles/Boing.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : examples/CMakeFiles/Boing.dir/depend
