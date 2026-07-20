# Chapter 5 — Generators and Commands

> This episode has almost no new *code* — instead we nail down the single most
> important idea in CMake (**it doesn't compile; it generates**), formally adopt
> **gcc + Ninja** as the repo's toolchain, and add two reference docs so you
> never have to guess a command again.
> **Branch:** `ep05-generators-commands`

## What you'll build

Nothing new gets compiled this episode. The program from Chapter 4 still builds
and runs exactly the same. What changes is your *understanding* and your
*paperwork*:

- You'll learn what a **generator** actually is, and why CMake needs one.
- You'll contrast **Ninja** and **Visual Studio** as two generators for the same
  `CMakeLists.txt`, and see why this book uses Ninja.
- You'll understand the **cache lock**: why the compiler and generator are
  chosen *once* and how to change your mind.
- You'll meet the classic `cannot execute 'as'` error and fix it for good.
- You'll get two new files committed to the repo: **`README.md`** (project
  overview) and **`COMMANDS.md`** (the full command reference).

## Where we left off

At the end of Chapter 4 the project looked like this: a `hello` executable built
from `Src/`, plus a `menus` library pulled in with `add_subdirectory` and linked
with `target_link_libraries`. The root `CMakeLists.txt` is unchanged this
episode:

```cmake
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
```

We've been typing this command since Chapter 1 without really explaining the
back half of it:

```bash
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
```

Time to explain the `-G Ninja` and `-D CMAKE_C_COMPILER=gcc` parts properly.

## The concept

### CMake does not compile your code

This is *the* idea to hold onto. CMake never turns a `.c` file into an
executable. It is a **generator**: it reads your `CMakeLists.txt` and writes out
build files for some *other* tool, and that tool does the actual compiling.

```
CMakeLists.txt ──▶ [ cmake generates ] ──▶ build files ──▶ [ build tool compiles ] ──▶ hello.exe
```

That's why every build is two steps, not one:

```bash
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc   # 1. configure = generate build files
cmake --build build                                    # 2. build     = the tool compiles
```

Step 1 produces build files. Step 2 hands those files to the build tool. The
`-G` flag is where you tell CMake **which build tool to generate for**.

### Two generators, same CMakeLists.txt

The beauty of writing `CMakeLists.txt` instead of hand-writing build files is
that *one* description can target *many* build systems. Pick the target with
`-G`:

| | `-G "Visual Studio 17 2022"` | `-G Ninja` |
|---|---|---|
| CMake writes | `.sln` + `.vcxproj` files | `build.ninja` |
| Compiler used | MSVC (`cl.exe`) via MSBuild | whatever you point at (gcc here) |
| Build tool | MSBuild | Ninja |
| Configs | multi-config (Debug/Release in one tree) | single-config (chosen at configure) |
| The exe lands at | `build/Debug/hello.exe` | `build/hello.exe` |
| Feel | IDE-oriented, heavier | fast, minimal, cross-platform |

Same source, same `CMakeLists.txt` — only `-G` differs, and CMake emits a
completely different set of build files.

### Why this book uses Ninja

We standardize on Ninja for a few concrete reasons:

- **Speed**, especially *incremental* builds. Change one file and Ninja rebuilds
  only what depends on it, near-instantly.
- **Cross-platform.** The same generator works on Windows, Linux, and macOS, so
  the commands in this book don't change per OS.
- **Clean progress output.** Ninja prints a tidy `[n/m]` counter (`[3/4]
  Building C object …`) so you can see exactly how far along a build is.
- **It's machine-generated.** `build.ninja` is meant to be *written by a tool*,
  not by a human — its syntax is deliberately dumb and verbose. That's the whole
  point of a generator: you write the readable `CMakeLists.txt`, and CMake writes
  the tedious `build.ninja` for you. You should never open `build.ninja` to edit
  it by hand.

### The cache lock

Both `-G <generator>` and `-D CMAKE_C_COMPILER=<gcc|clang>` are decisions CMake
makes **at the first configure** and then **caches** in `build/CMakeCache.txt`.
On every later `cmake --build build`, CMake reuses the cached choices — it does
*not* re-read those flags. That's a feature: the toolchain stays consistent for
the life of the `build/` directory.

The catch: you can't change your mind by just re-running configure with
different flags. To switch compiler or generator you must **start fresh** —
delete the build directory and reconfigure:

```bash
rm -rf build
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=clang     # different compiler
```

```bash
rm -rf build
cmake -S . -B build -G "Visual Studio 17 2022"             # different generator (MSVC)
```

If you skip the `rm -rf build`, CMake keeps the old cached toolchain and quietly
ignores your new flags.

### The toolchain must be on your PATH

gcc from mingw / w64devkit isn't a single self-contained program. `gcc.exe`
drives a small family of tools that live next to it — the assembler `as` and the
linker `ld`. CMake also needs `ninja`. All of them must be findable on your
`PATH`:

```bash
export PATH="/c/MinGW/w64devkit/bin:$PATH"
```

Skip this and you hit the single most common first-time error:

```text
gcc: fatal error: cannot execute 'as': CreateProcess: No such file or directory
```

Read that carefully — it's confusing until it clicks. gcc *did* run, so gcc
itself was found (CMake invoked it by full path). But gcc then tried to run its
sibling `as` **by bare name**, expecting to find it on PATH, and couldn't,
because the `w64devkit\bin` folder isn't on PATH. gcc found itself but not its
siblings.

The fix is to put that folder on PATH (the `export` line above), and add it to
your `~/.bashrc` so every new shell has it. One more step is easy to forget: if
you already tried to configure and it failed, CMake has **cached that the
compiler is broken**. Fixing PATH isn't enough — you must also nuke the build
dir so CMake re-tests the now-working compiler:

```bash
export PATH="/c/MinGW/w64devkit/bin:$PATH"   # fix PATH (and add to ~/.bashrc)
rm -rf build                                 # forget the "compiler broken" cache
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
```

## Step by step

The build graph does **not** change this episode. There are no new targets, no
edits to any `CMakeLists.txt`. We only:

1. **Add `README.md`** — a top-level overview of the whole project: what it is,
   how episodes-as-branches work, the toolchain, and a quickstart. It's the first
   thing a newcomer to the repo reads.
2. **Add `COMMANDS.md`** — a complete, copy-pasteable command reference:
   configure, build, run, test, and clean. This is the file you'll keep open in a
   tab for the rest of the book.
3. **Formally adopt gcc + Ninja** as *the* toolchain for every episode. From here
   on, the book assumes this combination.

That's the entire code delta — two new documentation files:

```text
 COMMANDS.md | 69 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 README.md   | 39 ++++++++++++++++++++++++++++++++++
 2 files changed, 108 insertions(+)
```

## The CMake, explained

There's no CMake *code* to explain this episode — but there is a command
reference to internalize. The two new docs are the deliverable, so let's read the
real content.

### `README.md` — the project overview

`README.md` orients anyone landing on the repo. Its key sections:

> This repo is built with **gcc (mingw / w64devkit) + Ninja**. That single
> toolchain builds every episode, including the cmocka unit tests introduced
> later. Episode 5 explains what a *generator* is and how you'd use Visual
> Studio / MSVC instead.

It reminds you to put the compiler on PATH…

```bash
export PATH="/c/MinGW/w64devkit/bin:$PATH"
```

…and gives the three-line quickstart:

```bash
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc   # configure (once)
cmake --build build                                    # compile
./build/hello.exe                                      # run
```

It also documents the episodes-as-branches model — each episode branch holds the
full working project as of that step, so you can `git checkout ep04-menus-library`
and build it — and points at `COMMANDS.md` for everything else.

### `COMMANDS.md` — the command reference

`COMMANDS.md` opens by restating the core idea in one line:

> The `cmake` chain is: CMake *generates* build files → the *generator* (Ninja)
> compiles them.

Here's the reference, section by section. This is the project's **enduring**
command list — a few sections (tests) describe tools we don't wire up until
Chapter 6, so treat those as forward references you'll grow into, not commands to
run at ep05.

**0. Once per shell — put the toolchain on PATH.** gcc needs its siblings
`as`/`ld`, and CMake needs `ninja`. Add it to `~/.bashrc` to avoid retyping.

```bash
export PATH="/c/MinGW/w64devkit/bin:$PATH"
```

**1. Configure (generate the build system) — occasionally.** Needed the first
time or after `rm -rf build`. The compiler + generator are locked into the cache
once set.

```bash
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
```

| Flag | Meaning |
|------|---------|
| `-S .` | source root (where the top `CMakeLists.txt` is) |
| `-B build` | output dir for generated build files |
| `-G Ninja` | the generator (which build tool to emit files for) |
| `-D CMAKE_C_COMPILER=gcc` | the C compiler to use |

**2. Build.** `--target` narrows the build to one thing:

```bash
cmake --build build                       # build everything
cmake --build build --target hello        # build only the app
cmake --build build --target test_math    # build only one test exe
```

**3. Run the app.**

```bash
./build/hello.exe
```

**4. Tests (from inside `build/`).** *(These arrive in Chapter 6 — CTest is the
test runner CMake generates for you.)*

```bash
cd build
ctest                        # run all tests, one line each
ctest --output-on-failure    # print output of tests that FAIL
ctest -V                     # verbose: all output, even passing
ctest -N                     # list tests without running
ctest -R math                # run only tests matching "math"
ctest --rerun-failed         # re-run last failures
```

**5. Run a test exe directly (raw cmocka output).**

```bash
./build/tests/test_math.exe
```

**6. Clean.** Know the difference between these two:

```bash
cmake --build build --target clean   # remove build outputs, keep config
rm -rf build                         # nuke everything, forces reconfigure
```

The everyday inner loop, once tests exist, is one line:

```bash
cmake --build build && (cd build && ctest --output-on-failure)
```

And the section that motivated this whole chapter — **changing the compiler /
generator** — restates the cache lock and the `rm -rf build` dance, and notes
that with the Visual Studio generator the exe lands in `build/Debug/hello.exe`
instead of `build/hello.exe`.

## Build and run

Behavior is identical to Chapter 4 — same graph, same output. Configure, build,
run:

```bash
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/hello.exe
```

```text
Hello, World!
2 + 3 = 5

=== Main Menu ===
1) Start
2) Settings
3) Quit

=== Settings ===
1) Volume
2) Brightness
3) Back
```

## What's autogenerated

Here's where the "CMake generates" idea becomes concrete. The `build/` directory
is *entirely* machine-written — it's in `.gitignore` and you never edit it by
hand. **What** gets written there depends on the generator:

| Generator | CMake writes into `build/` | Compiled by |
|-----------|----------------------------|-------------|
| `-G Ninja` | `build.ninja` (plus `CMakeCache.txt`, `CMakeFiles/`) | Ninja |
| `-G "Visual Studio 17 2022"` | `hello.sln`, `hello.vcxproj`, … | MSBuild |

Same `CMakeLists.txt`, same source files — different generated build files.
That's the generator doing its one job. With Ninja you get a single
`build/build.ninja`; with Visual Studio you get a solution and project files you
could open in the IDE. Either way, `CMakeCache.txt` is where the locked-in
compiler and generator live.

## Gotchas

- **Running `cmake` from a subfolder.** The `-S .` / `-B build` paths are
  relative to your current directory. Run the commands from the **project root**
  (as `COMMANDS.md` says up top), or you'll generate a `build/` in the wrong
  place and confuse yourself. Always `cd` back to the root first.
- **The cache remembers a broken or old compiler.** If configuring failed
  (e.g. the `cannot execute 'as'` error) or you want to *switch* compiler or
  generator, fixing PATH or changing the `-D`/`-G` flag is **not** enough — CMake
  reuses the cached choice. You must `rm -rf build` and reconfigure so CMake
  re-evaluates the toolchain from scratch.
- **Visual Studio puts the exe under `Debug/`, Ninja does not.** With
  `-G "Visual Studio 17 2022"` your program is at `build/Debug/hello.exe` because
  MSVC is multi-config. With Ninja (single-config) it's plain `build/hello.exe`.
  If `./build/hello.exe` says "not found," check which generator you configured
  with.

## Recap

- CMake is a **generator**: it does not compile — it writes build files for a
  build tool, which compiles. Hence the two-step configure-then-build flow.
- `-G` picks the generator. One `CMakeLists.txt` can target **Ninja**
  (`build.ninja`, fast, single-config, exe at `build/hello.exe`) or **Visual
  Studio** (`.sln`/`.vcxproj`, MSBuild, exe at `build/Debug/hello.exe`).
- The **compiler and generator are cached** at first configure. To change them,
  `rm -rf build` and reconfigure — new flags alone are ignored.
- The toolchain must be **on your PATH** (`export PATH="/c/MinGW/w64devkit/bin:$PATH"`);
  otherwise gcc can't find `as`/`ld` and you get `cannot execute 'as'`. Fix PATH,
  add it to `~/.bashrc`, and `rm -rf build` to clear the "broken compiler" cache.
- This episode added **`README.md`** (project overview) and **`COMMANDS.md`**
  (the full command reference), and formally adopted **gcc + Ninja**. No build
  graph changed.

## Get this episode

```bash
git checkout ep05-generators-commands
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/hello.exe
```

---
[← Chapter 4: Your first library module](04-menus-library.md) · [Chapter 6: Unit tests with cmocka + CTest →](06-cmocka-ctest.md)
