# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.12

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
CMAKE_COMMAND = /opt/clion-2018.2.5/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /opt/clion-2018.2.5/bin/cmake/linux/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/charlie/CLionProjects/OSTP2

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/charlie/CLionProjects/OSTP2/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/tp2_client.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/tp2_client.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/tp2_client.dir/flags.make

CMakeFiles/tp2_client.dir/client/client_thread.c.o: CMakeFiles/tp2_client.dir/flags.make
CMakeFiles/tp2_client.dir/client/client_thread.c.o: ../client/client_thread.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/charlie/CLionProjects/OSTP2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/tp2_client.dir/client/client_thread.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tp2_client.dir/client/client_thread.c.o   -c /home/charlie/CLionProjects/OSTP2/client/client_thread.c

CMakeFiles/tp2_client.dir/client/client_thread.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tp2_client.dir/client/client_thread.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/charlie/CLionProjects/OSTP2/client/client_thread.c > CMakeFiles/tp2_client.dir/client/client_thread.c.i

CMakeFiles/tp2_client.dir/client/client_thread.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tp2_client.dir/client/client_thread.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/charlie/CLionProjects/OSTP2/client/client_thread.c -o CMakeFiles/tp2_client.dir/client/client_thread.c.s

CMakeFiles/tp2_client.dir/client/main.c.o: CMakeFiles/tp2_client.dir/flags.make
CMakeFiles/tp2_client.dir/client/main.c.o: ../client/main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/charlie/CLionProjects/OSTP2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/tp2_client.dir/client/main.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tp2_client.dir/client/main.c.o   -c /home/charlie/CLionProjects/OSTP2/client/main.c

CMakeFiles/tp2_client.dir/client/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tp2_client.dir/client/main.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/charlie/CLionProjects/OSTP2/client/main.c > CMakeFiles/tp2_client.dir/client/main.c.i

CMakeFiles/tp2_client.dir/client/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tp2_client.dir/client/main.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/charlie/CLionProjects/OSTP2/client/main.c -o CMakeFiles/tp2_client.dir/client/main.c.s

CMakeFiles/tp2_client.dir/common/common.c.o: CMakeFiles/tp2_client.dir/flags.make
CMakeFiles/tp2_client.dir/common/common.c.o: ../common/common.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/charlie/CLionProjects/OSTP2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/tp2_client.dir/common/common.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tp2_client.dir/common/common.c.o   -c /home/charlie/CLionProjects/OSTP2/common/common.c

CMakeFiles/tp2_client.dir/common/common.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tp2_client.dir/common/common.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/charlie/CLionProjects/OSTP2/common/common.c > CMakeFiles/tp2_client.dir/common/common.c.i

CMakeFiles/tp2_client.dir/common/common.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tp2_client.dir/common/common.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/charlie/CLionProjects/OSTP2/common/common.c -o CMakeFiles/tp2_client.dir/common/common.c.s

# Object files for target tp2_client
tp2_client_OBJECTS = \
"CMakeFiles/tp2_client.dir/client/client_thread.c.o" \
"CMakeFiles/tp2_client.dir/client/main.c.o" \
"CMakeFiles/tp2_client.dir/common/common.c.o"

# External object files for target tp2_client
tp2_client_EXTERNAL_OBJECTS =

tp2_client: CMakeFiles/tp2_client.dir/client/client_thread.c.o
tp2_client: CMakeFiles/tp2_client.dir/client/main.c.o
tp2_client: CMakeFiles/tp2_client.dir/common/common.c.o
tp2_client: CMakeFiles/tp2_client.dir/build.make
tp2_client: CMakeFiles/tp2_client.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/charlie/CLionProjects/OSTP2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking C executable tp2_client"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/tp2_client.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/tp2_client.dir/build: tp2_client

.PHONY : CMakeFiles/tp2_client.dir/build

CMakeFiles/tp2_client.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/tp2_client.dir/cmake_clean.cmake
.PHONY : CMakeFiles/tp2_client.dir/clean

CMakeFiles/tp2_client.dir/depend:
	cd /home/charlie/CLionProjects/OSTP2/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/charlie/CLionProjects/OSTP2 /home/charlie/CLionProjects/OSTP2 /home/charlie/CLionProjects/OSTP2/cmake-build-debug /home/charlie/CLionProjects/OSTP2/cmake-build-debug /home/charlie/CLionProjects/OSTP2/cmake-build-debug/CMakeFiles/tp2_client.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/tp2_client.dir/depend

