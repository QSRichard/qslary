# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Produce verbose output by default.
VERBOSE = 1

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
CMAKE_SOURCE_DIR = /home/liushui/workspace/qslary/staticlibDymatic

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/liushui/workspace/qslary/staticlibDymatic

# Include any dependencies generated for this target.
include CMakeFiles/mytest.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/mytest.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/mytest.dir/flags.make

CMakeFiles/mytest.dir/test.cc.o: CMakeFiles/mytest.dir/flags.make
CMakeFiles/mytest.dir/test.cc.o: test.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/liushui/workspace/qslary/staticlibDymatic/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/mytest.dir/test.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/mytest.dir/test.cc.o -c /home/liushui/workspace/qslary/staticlibDymatic/test.cc

CMakeFiles/mytest.dir/test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/mytest.dir/test.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/liushui/workspace/qslary/staticlibDymatic/test.cc > CMakeFiles/mytest.dir/test.cc.i

CMakeFiles/mytest.dir/test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/mytest.dir/test.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/liushui/workspace/qslary/staticlibDymatic/test.cc -o CMakeFiles/mytest.dir/test.cc.s

# Object files for target mytest
mytest_OBJECTS = \
"CMakeFiles/mytest.dir/test.cc.o"

# External object files for target mytest
mytest_EXTERNAL_OBJECTS =

libmytest.a: CMakeFiles/mytest.dir/test.cc.o
libmytest.a: CMakeFiles/mytest.dir/build.make
libmytest.a: CMakeFiles/mytest.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/liushui/workspace/qslary/staticlibDymatic/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libmytest.a"
	$(CMAKE_COMMAND) -P CMakeFiles/mytest.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mytest.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/mytest.dir/build: libmytest.a

.PHONY : CMakeFiles/mytest.dir/build

CMakeFiles/mytest.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/mytest.dir/cmake_clean.cmake
.PHONY : CMakeFiles/mytest.dir/clean

CMakeFiles/mytest.dir/depend:
	cd /home/liushui/workspace/qslary/staticlibDymatic && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/liushui/workspace/qslary/staticlibDymatic /home/liushui/workspace/qslary/staticlibDymatic /home/liushui/workspace/qslary/staticlibDymatic /home/liushui/workspace/qslary/staticlibDymatic /home/liushui/workspace/qslary/staticlibDymatic/CMakeFiles/mytest.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/mytest.dir/depend

