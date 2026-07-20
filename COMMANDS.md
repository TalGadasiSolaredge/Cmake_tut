# Command Reference

All commands run from the **project root** unless noted. The `cmake` chain is:
CMake *generates* build files → the *generator* (Ninja) compiles them.

## 0. Once per shell — put the toolchain on PATH
```bash
export PATH="/c/MinGW/w64devkit/bin:$PATH"
```
gcc needs its siblings `as`/`ld`, and CMake needs `ninja`. Add it to
`~/.bashrc` to avoid retyping.

## 1. Configure (generate the build system) — occasionally
```bash
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
```
`-S .` = source root, `-B build` = output dir, `-G Ninja` = generator,
`-D CMAKE_C_COMPILER=gcc` = compiler. Needed the first time or after
`rm -rf build`. The compiler + generator are locked into the cache once set.

## 2. Build
```bash
cmake --build build                       # build everything
cmake --build build --target hello        # build only the app
cmake --build build --target test_math    # build only one test exe
```

## 3. Run the app
```bash
./build/hello.exe
```

## 4. Tests (from inside build/)
```bash
cd build
ctest                        # run all tests, one line each
ctest --output-on-failure    # print output of tests that FAIL
ctest -V                     # verbose: all output, even passing
ctest -N                     # list tests without running
ctest -R math                # run only tests matching "math"
ctest --rerun-failed         # re-run last failures
```

## 5. Run a test exe directly (raw cmocka output)
```bash
./build/tests/test_math.exe
```

## 6. Clean
```bash
cmake --build build --target clean   # remove build outputs, keep config
rm -rf build                         # nuke everything, forces reconfigure
```

## The everyday loop
```bash
cmake --build build && (cd build && ctest --output-on-failure)
```

## Changing the compiler / generator
The compiler and generator are chosen at first configure and cached. To switch,
start fresh:
```bash
rm -rf build
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=clang     # different compiler
cmake -S . -B build -G "Visual Studio 17 2022"            # different generator (MSVC)
```
With the Visual Studio generator the exe lands in `build/Debug/hello.exe`
instead of `build/hello.exe`.
