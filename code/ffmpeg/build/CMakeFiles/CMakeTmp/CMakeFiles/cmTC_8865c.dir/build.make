# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.24

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

# Produce verbose output by default.
VERBOSE = 1

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = D:\Study\cmake-3.24.0-rc2-windows-x86_64\bin\cmake.exe

# The command to remove a file.
RM = D:\Study\cmake-3.24.0-rc2-windows-x86_64\bin\cmake.exe -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = D:\Study\code\ffmpeg\build\CMakeFiles\CMakeTmp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = D:\Study\code\ffmpeg\build\CMakeFiles\CMakeTmp

# Include any dependencies generated for this target.
include CMakeFiles/cmTC_8865c.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/cmTC_8865c.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/cmTC_8865c.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/cmTC_8865c.dir/flags.make

CMakeFiles/cmTC_8865c.dir/CMakeCCompilerABI.c.obj: CMakeFiles/cmTC_8865c.dir/flags.make
CMakeFiles/cmTC_8865c.dir/CMakeCCompilerABI.c.obj: D:/Study/cmake-3.24.0-rc2-windows-x86_64/share/cmake-3.24/Modules/CMakeCCompilerABI.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --progress-dir=D:\Study\code\ffmpeg\build\CMakeFiles\CMakeTmp\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/cmTC_8865c.dir/CMakeCCompilerABI.c.obj"
	D:\Software\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles\cmTC_8865c.dir\CMakeCCompilerABI.c.obj -c D:\Study\cmake-3.24.0-rc2-windows-x86_64\share\cmake-3.24\Modules\CMakeCCompilerABI.c

CMakeFiles/cmTC_8865c.dir/CMakeCCompilerABI.c.i: cmake_force
	@echo Preprocessing C source to CMakeFiles/cmTC_8865c.dir/CMakeCCompilerABI.c.i
	D:\Software\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E D:\Study\cmake-3.24.0-rc2-windows-x86_64\share\cmake-3.24\Modules\CMakeCCompilerABI.c > CMakeFiles\cmTC_8865c.dir\CMakeCCompilerABI.c.i

CMakeFiles/cmTC_8865c.dir/CMakeCCompilerABI.c.s: cmake_force
	@echo Compiling C source to assembly CMakeFiles/cmTC_8865c.dir/CMakeCCompilerABI.c.s
	D:\Software\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S D:\Study\cmake-3.24.0-rc2-windows-x86_64\share\cmake-3.24\Modules\CMakeCCompilerABI.c -o CMakeFiles\cmTC_8865c.dir\CMakeCCompilerABI.c.s

# Object files for target cmTC_8865c
cmTC_8865c_OBJECTS = \
"CMakeFiles/cmTC_8865c.dir/CMakeCCompilerABI.c.obj"

# External object files for target cmTC_8865c
cmTC_8865c_EXTERNAL_OBJECTS =

cmTC_8865c.exe: CMakeFiles/cmTC_8865c.dir/CMakeCCompilerABI.c.obj
cmTC_8865c.exe: CMakeFiles/cmTC_8865c.dir/build.make
cmTC_8865c.exe: CMakeFiles/cmTC_8865c.dir/objects1.rsp
cmTC_8865c.exe: CMakeFiles/cmTC_8865c.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --progress-dir=D:\Study\code\ffmpeg\build\CMakeFiles\CMakeTmp\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable cmTC_8865c.exe"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\cmTC_8865c.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/cmTC_8865c.dir/build: cmTC_8865c.exe
.PHONY : CMakeFiles/cmTC_8865c.dir/build

CMakeFiles/cmTC_8865c.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\cmTC_8865c.dir\cmake_clean.cmake
.PHONY : CMakeFiles/cmTC_8865c.dir/clean

CMakeFiles/cmTC_8865c.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" D:\Study\code\ffmpeg\build\CMakeFiles\CMakeTmp D:\Study\code\ffmpeg\build\CMakeFiles\CMakeTmp D:\Study\code\ffmpeg\build\CMakeFiles\CMakeTmp D:\Study\code\ffmpeg\build\CMakeFiles\CMakeTmp D:\Study\code\ffmpeg\build\CMakeFiles\CMakeTmp\CMakeFiles\cmTC_8865c.dir\DependInfo.cmake
.PHONY : CMakeFiles/cmTC_8865c.dir/depend

