# Minimum CMake version required to process this file.
cmake_minimum_required(VERSION 3.15)

# Project name and the language(s) it uses.
project(hello LANGUAGES C)

# Write build/compile_commands.json listing the exact compile flags for every
# source file. VS Code's C/C++ extension reads it so IntelliSense knows the
# include paths (stdint.h, cmocka.h, ...). Ninja/Makefile generators only.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# A user-settable build option. Default OFF. Flip it at configure time with:
#   cmake -S . -B build -D ENABLE_DEBUG_MENU=ON
# The value is remembered in the CMake cache until you change it again.
option(ENABLE_DEBUG_MENU "Include the debug menu screen" OFF)

# Pull in the "menus" subfolder. CMake runs menus/CMakeLists.txt,
# which defines the "menus" library target.
add_subdirectory(menus)

# Pull in the "adc" module (defines the "adc" library target).
add_subdirectory(adc)

# Build an executable called "hello" from these source files (all under Src/).
add_executable(hello
    Src/main.c
    Src/math_utils.c
)

# Tell the compiler where to find this project's own headers (Inc/).
target_include_directories(hello PRIVATE Inc)

# Link the "menus" library into the app. Because menus' Inc was marked PUBLIC,
# the app automatically gets menus' header search path too.
target_link_libraries(hello PRIVATE menus adc)

# Turn on CTest support, then pull in the tests folder.
enable_testing()
add_subdirectory(tests)
