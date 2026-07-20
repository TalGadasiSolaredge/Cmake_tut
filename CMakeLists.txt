# Minimum CMake version required to process this file.
cmake_minimum_required(VERSION 3.15)

# Project name and the language(s) it uses.
project(hello LANGUAGES C)

# Build an executable called "hello" from these source files (all under Src/).
add_executable(hello
    Src/main.c
    Src/math_utils.c
)

# Headers live under Inc/ -- tell the compiler where to find them so that
# #include "math_utils.h" resolves even though the .h is in a different folder.
target_include_directories(hello PRIVATE Inc)
