# Minimum CMake version required to process this file.
cmake_minimum_required(VERSION 3.15)

# Project name and the language(s) it uses.
project(hello LANGUAGES C)

# Build an executable called "hello" from these source files.
# Add every .c file here. Headers (.h) do NOT go in this list.
add_executable(hello main.c math_utils.c)
