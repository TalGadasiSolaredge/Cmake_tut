# Appendix A — Command Reference

Every command runs from the **project root** unless noted. First put the
toolchain on PATH (once per shell):

```bash
export PATH="/c/MinGW/w64devkit/bin:$PATH"
```

## Configure (generate the build system)
```bash
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
```
`-S .` source dir · `-B build` output dir · `-G Ninja` generator ·
`-D CMAKE_C_COMPILER=gcc` compiler. Needed the first time, or after
`rm -rf build`. Compiler + generator are locked into the cache once set.

## Build
```bash
cmake --build build                       # build everything
cmake --build build --target hello        # build only the app
cmake --build build --target test_math    # build only one test exe
```

## Run the app
```bash
./build/hello.exe
```

## Tests — from inside `build/`
```bash
cd build
ctest                        # run all tests, one line each
ctest --output-on-failure    # print output of tests that FAIL
ctest -V                     # verbose: all output, even passing
ctest -N                     # list tests without running
ctest -R math                # run only tests matching "math"
ctest --rerun-failed         # re-run last failures
```

## Run a test exe directly (raw cmocka output)
```bash
./build/tests/test_math.exe
./build/tests/test_menus.exe
./build/tests/test_adc.exe
```

## Build options (Chapters 8–9)
```bash
cmake -S . -B build -D ENABLE_DEBUG_MENU=ON       # turn a flag on
cmake -S . -B build -D VERBOSE_LOGGING=ON -D ADC_VREF_MV=3300
cmake -LAH build                                  # list all cached options
```
Remember: option values live in `build/CMakeCache.txt` and persist until you
change them again or delete `build/`.

## Clean
```bash
cmake --build build --target clean   # remove build outputs, keep config
rm -rf build                         # nuke everything, forces reconfigure
```

## The everyday loop
```bash
cmake --build build && (cd build && ctest --output-on-failure)
```

## Changing compiler / generator
```bash
rm -rf build
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=clang    # different compiler
cmake -S . -B build -G "Visual Studio 17 2022"           # different generator
```
With the Visual Studio generator the exe is at `build/Debug/hello.exe` instead
of `build/hello.exe`.

---
[← Chapter 10: Library-to-library dependencies](10-lib-plus-lib.md) · [Appendix B: cmocka vs CTest →](appendix-b-cmocka-vs-ctest.md)
