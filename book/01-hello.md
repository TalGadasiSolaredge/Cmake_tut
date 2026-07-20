# Chapter 1 — Minimal CMake Hello World

> Build and run the smallest possible CMake project — an executable that prints one line — so you can see the two-step CMake workflow with nothing else in the way. **Branch:** `ep01-hello`

## What you'll build

A C program that prints `Hello, World!`, driven by a `CMakeLists.txt` that is only three real lines long. That's the whole project — two files:

| File | Purpose |
|------|---------|
| `main.c` | The C source. Prints one line and exits. |
| `CMakeLists.txt` | Tells CMake what to build. |

By the end you'll have configured the project, built it, and run the resulting `build/hello.exe`. More importantly, you'll understand *why* there are two separate steps, because that idea underpins everything else in this book.

## Starting from scratch

Everything you need lives in the two files above. There is no `src/` folder, no library, no options — just a source file and the minimal recipe to compile it. We're keeping the surface area tiny on purpose: when the project is this small, there's nowhere for confusion to hide.

Here is the complete `main.c`:

```c
#include <stdio.h>

int main(void)
{
    printf("Hello, World!\n");
    return 0;
}
```

Nothing CMake-specific here — it's ordinary C. The interesting part is how we ask CMake to turn it into a runnable program.

## The concept

The single most important idea in this whole book:

**CMake does not compile your code. It is a *generator*.**

CMake reads your `CMakeLists.txt` and writes out build files for some *other* tool. That other tool is the one that actually invokes the compiler. In this book the other tool is **Ninja**.

```text
CMakeLists.txt ──▶ [ cmake generates ] ──▶ build files ──▶ [ Ninja builds ] ──▶ hello.exe
```

Because of this, there are always **two steps**:

1. **Configure** (generate): CMake reads `CMakeLists.txt` and writes build files into a `build/` folder.
2. **Build** (compile): the build tool reads those generated files and produces the executable.

```bash
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc   # 1. configure (generate)
cmake --build build                                    # 2. build (compile)
```

Let's decode that first command, because you'll type it constantly:

| Flag | Meaning |
|------|---------|
| `-S .` | **S**ource directory — where `CMakeLists.txt` lives. `.` is the current folder (the project root). |
| `-B build` | **B**uild directory — where CMake writes everything it generates. It creates `build/` if it doesn't exist. |
| `-G Ninja` | **G**enerator — which build tool to generate files for. Here, Ninja. |
| `-D CMAKE_C_COMPILER=gcc` | **D**efine a CMake variable. This one tells CMake to use `gcc` as the C compiler. |

The second command, `cmake --build build`, is a portable way to say "run the build tool on the `build/` folder." It just calls Ninja for you, so you don't have to remember Ninja's own command line.

## Step by step

1. Get onto the episode branch and put the toolchain on your PATH (details in the [Get this episode](#get-this-episode) block below).
2. **Configure once:** `cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc`. This creates and fills the `build/` folder.
3. **Build:** `cmake --build build`. This compiles `main.c` and links it into `build/hello.exe`.
4. **Run:** `./build/hello.exe`.

You only need to re-run step 2 when you change something structural in `CMakeLists.txt`. For everyday code edits you just re-run step 3 — Ninja figures out what changed and recompiles the minimum.

## The CMake, explained

Here is the entire `CMakeLists.txt`, exactly as committed on this branch:

```cmake
# Minimum CMake version required to process this file.
cmake_minimum_required(VERSION 3.15)

# Project name and the language(s) it uses.
project(hello LANGUAGES C)

# Build an executable called "hello" from main.c.
add_executable(hello main.c)
```

Three real lines (the rest are comments). Taken one at a time:

- `cmake_minimum_required(VERSION 3.15)` — declares the oldest CMake version that understands this file. If someone runs an older CMake, they get a clear error instead of a mysterious failure. It should be the first line of every `CMakeLists.txt`.

- `project(hello LANGUAGES C)` — names the project `hello` and tells CMake which languages it uses. `LANGUAGES C` means "this is a C project" — so CMake goes looking for a C compiler (and doesn't waste time hunting for a C++ one). This is the point where CMake detects and validates your compiler.

- `add_executable(hello main.c)` — the payoff line. It says: build an executable **target** named `hello` from the source file `main.c`. The first argument is the target name; everything after is source files. On Windows the produced file gets an `.exe` suffix automatically, so you end up with `hello.exe`.

That's genuinely all CMake needs to build a program.

## Build and run

Run the two steps, then the executable:

```bash
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/hello.exe
```

Expected output:

```text
Hello, World!
```

Note the path: with the Ninja generator the executable lands directly at `build/hello.exe`. There is **no** `Debug/` or `Release/` subfolder — that layout is a Visual Studio thing, and we cover it in Chapter 5.

## What's autogenerated

You wrote two files. CMake wrote everything else. The entire `build/` folder is generated output — you never edit it by hand and you never commit it. (On this branch, `.gitignore` contains `/build/`, so git ignores it for you.)

The interesting things CMake drops into `build/`:

| Path | What it is |
|------|-----------|
| `build/CMakeCache.txt` | Cached configuration — the compiler it found, your `-D` options, and more. This is why you configure *once* and build many times: the answers are remembered here. |
| `build/CMakeFiles/` | CMake's internal bookkeeping — feature-detection results, dependency info, scratch files. You can ignore it. |
| `build/build.ninja` | The actual build file CMake generated for Ninja. This is the "output" of the generator step — the recipe Ninja follows. |
| `build/cmake_install.cmake` | The script that would run on `cmake --install` (not something we use yet). |
| `build/hello.exe` | Your compiled program, produced by the build step. |

Because it's all regenerated, `build/` is completely disposable. Delete it and reconfigure and you get an identical result:

```bash
rm -rf build
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
```

If your build ever gets into a weird state, wiping `build/` and reconfiguring is the first thing to try.

## Gotchas

- **Run `cmake` from the project root**, not from inside a subfolder. `-S .` points at the current directory, so CMake expects to find `CMakeLists.txt` right there. Configuring from the wrong directory means CMake can't find the source (or finds the wrong one).

- **The toolchain must be on your PATH.** `gcc` doesn't work alone — it shells out to its siblings `as` (the assembler) and `ld` (the linker). If they aren't on PATH you'll see an error like `gcc: fatal error: cannot execute 'as'`. Put the toolchain's `bin/` directory on PATH first:

  ```bash
  export PATH="/c/MinGW/w64devkit/bin:$PATH"
  ```

  Chapter 5 goes into detail on toolchains, generators, and why this matters.

## Recap

- CMake is a **generator**, not a compiler. It reads `CMakeLists.txt` and writes build files for another tool (here, Ninja).
- The workflow is always two steps: **configure** (`cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc`) then **build** (`cmake --build build`).
- `-S .` is the source dir, `-B build` is the output dir, `-G` picks the generator, `-D` sets a variable.
- Three lines do the work: `cmake_minimum_required`, `project`, and `add_executable(hello main.c)`.
- The whole `build/` folder is autogenerated and never committed; delete it any time and it regenerates.

## Get this episode

```bash
git checkout ep01-hello
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/hello.exe
```

---
[← Chapter 0: Introduction](00-introduction.md) · [Chapter 2: A header and its source file →](02-header-source.md)
