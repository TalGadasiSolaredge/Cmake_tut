# CMake by Example

A tiny C project that grows, one episode at a time, to teach CMake from a
3-line hello world up to libraries, unit tests, mocking, build options, and a
generated config header.

Each episode is a git branch holding the **full working project** as of that
step, so you can check one out and build it:

```bash
git checkout ep04-menus-library
```

The written walkthrough for every episode lives in the [`book/`](book/) folder
on the `main` branch. A rendered, single-file version is also here as
[`CMake-by-Example.pdf`](CMake-by-Example.pdf) (99 pages).

### Rebuild the PDF

```bash
bash tools/make-pdf.sh      # needs Node.js + Chrome or Edge
```

This converts `book/*.md` to a styled HTML book (via `marked`) and prints it to
`CMake-by-Example.pdf` with a headless browser.

## Toolchain

This repo is built with **gcc (mingw / w64devkit) + Ninja**. That single
toolchain builds every episode, including the cmocka unit tests introduced
later. Episode 5 explains what a *generator* is and how you'd use Visual
Studio / MSVC instead.

Make sure the compiler is on your PATH (adjust to your install):

```bash
export PATH="/c/MinGW/w64devkit/bin:$PATH"
```

## Build & run

```bash
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc   # configure (once)
cmake --build build                                    # compile
./build/hello.exe                                      # run
```

See [`COMMANDS.md`](COMMANDS.md) for the full command reference (building,
running, testing, cleaning).
