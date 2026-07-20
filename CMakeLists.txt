# Minimum CMake version required to process this file.
cmake_minimum_required(VERSION 3.15)

# Project name, VERSION (feeds APP_VERSION below), and language(s).
project(hello VERSION 1.2.0 LANGUAGES C)

# Write build/compile_commands.json listing the exact compile flags for every
# source file. VS Code's C/C++ extension reads it so IntelliSense knows the
# include paths (stdint.h, cmocka.h, ...). Ninja/Makefile generators only.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =========================================================================
#  Build configuration flags (5). Change them at configure time with -D:
#    cmake -S . -B build -D VERBOSE_LOGGING=ON -D ADC_VREF_MV=3300
#  Values are cached until changed again (or the build dir is deleted).
# =========================================================================
option(ENABLE_DEBUG_MENU     "Include the debug menu screen"        OFF)
option(ENABLE_STARTUP_BANNER "Print the startup banner"             ON)
option(VERBOSE_LOGGING       "Emit extra verbose logging"           OFF)
set(ADC_VREF_MV 3000 CACHE STRING "ADC reference voltage in millivolts")
# 5th flag, APP_VERSION, is derived from PROJECT_VERSION (set by project()).

# Fill in app_config.h.in -> build/generated/app_config.h with the values above.
configure_file(
    ${CMAKE_SOURCE_DIR}/config/app_config.h.in
    ${CMAKE_BINARY_DIR}/generated/app_config.h
)

# A header-only (INTERFACE) target whose only job is to hand out the include
# path of the generated header. Any target that links app_config can then just
# #include "app_config.h". This is the tidy way to distribute a generated file.
add_library(app_config INTERFACE)
target_include_directories(app_config INTERFACE ${CMAKE_BINARY_DIR}/generated)

# math_utils is now a library module too. It must come before menus, which
# links it.
add_subdirectory(math)

# Pull in the "menus" subfolder. CMake runs menus/CMakeLists.txt,
# which defines the "menus" library target.
add_subdirectory(menus)

# Pull in the "adc" module (defines the "adc" library target).
add_subdirectory(adc)

# Build an executable called "hello". The app is now just main.c -- every other
# piece of code lives in a library.
add_executable(hello
    Src/main.c
)

# Link the libraries into the app. Each library brings its own PUBLIC include
# dirs, so the app needs no target_include_directories of its own anymore:
#   math_utils -> math_utils.h    menus/adc -> their headers
#   app_config -> generated app_config.h
target_link_libraries(hello PRIVATE math_utils menus adc app_config)

# Turn on CTest support, then pull in the tests folder.
enable_testing()
add_subdirectory(tests)
