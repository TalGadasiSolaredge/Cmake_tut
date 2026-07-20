# Minimum CMake version required to process this file.
cmake_minimum_required(VERSION 3.15)

# Project name and the language(s) it uses.
project(hello LANGUAGES C)

# Pull in the "menus" subfolder. CMake runs menus/CMakeLists.txt,
# which defines the "menus" library target.
add_subdirectory(menus)

# Build an executable called "hello" from these source files (all under Src/).
add_executable(hello
    Src/main.c
    Src/math_utils.c
)

# Tell the compiler where to find this project's own headers (Inc/).
target_include_directories(hello PRIVATE Inc)

# Link the "menus" library into the app. Because menus' Inc was marked PUBLIC,
# the app automatically gets menus' header search path too.
target_link_libraries(hello PRIVATE menus)
