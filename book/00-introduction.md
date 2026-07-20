# Chapter 0 — Introduction & Setup

## What this book is

CMake has a reputation for being confusing, mostly because tutorials dump a big
`CMakeLists.txt` on you and explain nothing. This book does the opposite: we
start with the **smallest possible** CMake project — three lines — and add
exactly one idea per episode, building a real (if small) C application as we go.

By the end you'll have used, and understood, the CMake features you actually
reach for day to day:

- building executables and libraries
- splitting a project into folders and modules
- include paths and the PUBLIC/PRIVATE model
- generators (Ninja vs Visual Studio) and choosing a compiler
- unit testing with cmocka and CTest
- mocking hardware for host-side tests
- build options and preprocessor defines
- generated configuration headers

## What CMake actually is (the one idea to hold onto)

**CMake does not compile your code.** It is a *generator*: it reads your
`CMakeLists.txt` and writes out build files for some other tool, which then does
the real compiling.

```
CMakeLists.txt ──▶ [ cmake generates ] ──▶ build files ──▶ [ build tool ] ──▶ your program
```

In this book the build tool is **Ninja**. So there are always two steps:

```bash
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc   # 1. configure (generate)
cmake --build build                                    # 2. build (compile)
```

Chapter 5 digs into what a generator is and how you'd swap Ninja for Visual
Studio's MSBuild.

## Prerequisites / toolchain

This project is built and tested with:

| Tool | What / where (this machine) |
|------|------------------------------|
| CMake | 3.15+ (any recent version) |
| Compiler | gcc — mingw / w64devkit, at `C:\MinGW\w64devkit\bin` |
| Generator/build tool | Ninja |
| Test framework | cmocka — a mingw build at `C:\MinGW\cmocka` (from Chapter 6 on) |

**Put the compiler toolchain on your PATH** before running CMake — gcc needs its
siblings `as` and `ld`, and CMake needs `ninja`:

```bash
export PATH="/c/MinGW/w64devkit/bin:$PATH"
```

If you skip this you'll see `gcc: fatal error: cannot execute 'as'` — that's the
symptom of the toolchain not being on PATH (see Chapter 5). Add the line to your
`~/.bashrc` so every shell has it.

> The cmocka paths (`C:\MinGW\cmocka`) are hard-coded in the test CMake files
> because that's where it lives on this machine. On another machine, adjust
> `CMOCKA_DIR` in `tests/CMakeLists.txt`.

## How the episodes work

The `main` branch has this book and the final code. Each **episode is its own
branch** with the full project as it stood at the end of that episode:

```bash
git checkout ep01-hello        # the 3-line starting point
git checkout ep10-lib-plus-lib # the finished project
git checkout main              # back to the latest + this book
```

Read the chapter, check out the branch, build it, poke at it. Onward to
[Chapter 1](01-hello.md).
