# Chapter 3 — Organizing into Inc/ and Src/

> Move headers into `Inc/` and sources into `Src/`, then learn the one new command that keeps `#include` working after the move: `target_include_directories`. **Branch:** `ep03-inc-src-layout`

## What you'll build

The exact same program as Chapter 2 — it still prints a greeting and adds two
numbers — but the files are now organized the way real C projects lay them out:
public headers in an `Inc/` folder, source files in a `Src/` folder.

```text
cmake-course/
├── CMakeLists.txt
├── .gitignore
├── Inc/
│   └── math_utils.h
└── Src/
    ├── main.c
    └── math_utils.c
```

Nothing about *what the program does* changes. This episode is entirely about
**layout**, and about the one CMake command you need to add so the compiler can
still find your header after you move it.

## Where we left off

In Chapter 2 every file sat flat in the repository root — `main.c`,
`math_utils.c`, and `math_utils.h` were all next to each other:

```text
cmake-course/
├── CMakeLists.txt
├── main.c
├── math_utils.c
└── math_utils.h
```

And the CMake was short:

```cmake
add_executable(hello main.c math_utils.c)
```

That worked, and importantly `#include "math_utils.h"` in `main.c` resolved
**for free**. Why? Because the header was sitting right next to the `.c` files
that included it. A quoted `#include` looks in the folder of the current file
first, so the compiler found `math_utils.h` without us telling it anything.

The moment we move the header into its own folder, that free lookup stops
working — and that is the whole lesson of this chapter.

## The concept

### The compiler's header search path

When the compiler hits a line like:

```c
#include "math_utils.h"
```

it has to go find a file called `math_utils.h` on disk. It does not search your
whole computer. It searches a specific, ordered list of folders called the
**header search path** (also called the *include path*).

For a quoted include (`"..."`), the compiler looks **next to the current source
file first**, then falls through to the directories on the search path. In
Chapter 2 the "next to the current file" step was enough, because the header
lived in the same folder as `main.c`.

Now `main.c` is in `Src/` and `math_utils.h` is in `Inc/`. They are in
different folders. "Next to `main.c`" no longer contains the header, and `Inc/`
is not on the search path yet — so the include fails.

### Putting `Inc/` back on the path

The fix is to add `Inc/` to the header search path. In CMake, you do that with
one command:

```cmake
target_include_directories(hello PRIVATE Inc)
```

Read it as: *"When building the target `hello`, add the folder `Inc` to the list
of places the compiler looks for headers."* Now `#include "math_utils.h"`
resolves again, because `Inc/` is on the path and `math_utils.h` lives there.

### Headers are still not build sources

This is worth repeating from Chapter 2, because moving the header into `Inc/`
can make it *feel* like it should now be "added" somewhere new. It should not.

Look at `add_executable` — it lists only `.c` files. `math_utils.h` appears
**nowhere** as a thing to compile. What we did with `target_include_directories`
is completely different: we told the compiler **where to find** the header, not
to compile it. Headers are found and pasted in by `#include`; they are never
compiled on their own.

So the two jobs are cleanly separated:

- `add_executable(hello Src/main.c Src/math_utils.c)` — *what to compile* (the `.c` files).
- `target_include_directories(hello PRIVATE Inc)` — *where to look* for the `.h` files those sources include.

### What `PRIVATE` means here

`target_include_directories` takes a keyword — here `PRIVATE`. It answers the
question: *"Who gets to use this include directory?"*

`PRIVATE` means: **this include directory is used only when building `hello`
itself.** Nobody else. That is exactly right for an executable — nothing links
*against* `hello`, so there is no "anyone else" to share the path with.

The keyword only starts to earn its keep once you have libraries, where one
target is built *and then used by* another target. When we build our first
library in Chapter 4, we'll see why a library might want `PUBLIC` (so code that
links the library also gets its headers on the search path) versus `PRIVATE`
(headers the library needs internally but that its users should not see). For an
executable, `PRIVATE` is the natural choice, so that's what we use.

## Step by step

**1. Create the two folders and move the files.**

- `math_utils.h` → `Inc/math_utils.h`
- `main.c` → `Src/main.c`
- `math_utils.c` → `Src/math_utils.c`

The *contents* of the files do not change at all — only their location. Here
they are for reference.

`Inc/math_utils.h`:

```c
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

/* Returns the sum of a and b. */
int add(int a, int b);

#endif /* MATH_UTILS_H */
```

`Src/main.c`:

```c
#include <stdio.h>
#include "math_utils.h"

int main(void)
{
    printf("Hello, World!\n");
    printf("2 + 3 = %d\n", add(2, 3));
    return 0;
}
```

`Src/math_utils.c`:

```c
#include "math_utils.h"

int add(int a, int b)
{
    return a + b;
}
```

Notice that `main.c` still writes `#include "math_utils.h"` — just the bare file
name, not `#include "../Inc/math_utils.h"`. We do **not** hard-code the folder
into the source. Instead we let CMake add `Inc/` to the search path, which keeps
the source clean and portable. That's the whole point of an include directory.

**2. Update `CMakeLists.txt`** so the source paths point into `Src/`, and add
the new `target_include_directories` line so `Inc/` is on the header search
path.

## The CMake, explained

Here is the complete `CMakeLists.txt` for this episode:

```cmake
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
```

Two things changed from Chapter 2:

- **The source paths are now folder-qualified.** `add_executable` lists
  `Src/main.c` and `Src/math_utils.c` instead of the bare `main.c` /
  `math_utils.c`. These paths are relative to the folder containing
  `CMakeLists.txt` (the project root), so CMake resolves them straight to the
  files inside `Src/`. We also spread the list across multiple lines — CMake
  does not care about the whitespace, and it reads more clearly once you have a
  couple of sources.

- **A new command appeared:** `target_include_directories(hello PRIVATE Inc)`.
  This is the line doing the real work of this chapter. `Inc` is again relative
  to the project root, so it points at our `Inc/` folder. Adding it to `hello`'s
  header search path is what makes `#include "math_utils.h"` resolve again.

Note the shape of the command: it is `target_`-something and its first argument
is a target (`hello`). CMake has a whole family of `target_*` commands
(`target_include_directories`, `target_link_libraries`,
`target_compile_definitions`, ...) that all attach a setting **to a specific
target** rather than to the whole project. This is the modern, recommended way
to configure things in CMake, and you'll meet more of these commands in later
chapters.

## Build and run

The build steps are identical to before — configure, then build. From the
project root:

```bash
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/hello.exe
```

Expected output:

```text
Hello, World!
2 + 3 = 5
```

Exactly what Chapter 2 printed — because only the layout changed, not the
behavior. If you get that output, the reorganization worked and CMake found the
header in its new home.

## What's autogenerated

Nothing new this episode. As in Chapter 1, `cmake -S . -B build` fills the
`build/` folder with generated files — `build.ninja`, `CMakeCache.txt`, the
`CMakeFiles/` bookkeeping directory — and `cmake --build build` produces
`hello.exe` there. You still never edit anything under `build/`, and `build/` is
still git-ignored. Moving your own files into `Inc/` and `Src/` doesn't add any
new categories of generated output; it just changes where *your* files live.

## Gotchas

- **Forgetting `target_include_directories` after moving the header.** This is
  the classic first stumble with this layout. You move `math_utils.h` into
  `Inc/`, update the `Src/` paths in `add_executable`, build — and it fails
  with:

  ```text
  fatal error: math_utils.h: No such file or directory
  ```

  The compiler is telling you plainly: it looked, and `Inc/` was not on the
  search path, so it couldn't find the header. The fix is the one line this
  chapter is about — `target_include_directories(hello PRIVATE Inc)`. If you see
  a `No such file or directory` on a header, "is its folder on the include
  path?" is the first question to ask.

- **Trying to "fix" it in the source instead.** It's tempting to change the
  include to `#include "../Inc/math_utils.h"` to make the error go away. Don't.
  That hard-codes the relative layout into your source and breaks the moment the
  file moves again. The include directory belongs in the build configuration
  (`CMakeLists.txt`), not baked into every `#include`.

- **Adding the `.h` to `add_executable`.** Also tempting, also wrong — and it
  won't even fix the error. Headers are never listed as sources; the search path
  is what resolves them.

## Recap

- Real projects separate **headers** (`Inc/`) from **sources** (`Src/`); this
  episode adopts that layout without changing the program's behavior.
- A quoted `#include` is resolved against the compiler's **header search path**.
  It searches next to the current file first, which is why a flat layout "just
  worked" in Chapter 2.
- Once the header lives in a different folder, you must add that folder to the
  search path with **`target_include_directories(hello PRIVATE Inc)`**.
- Headers are still **never** build sources. `add_executable` lists only `.c`
  files; `target_include_directories` tells the compiler where to *find* the
  `.h` files, not to compile them.
- **`PRIVATE`** means the include directory is used only to build `hello` — the
  distinction between `PUBLIC` and `PRIVATE` becomes important once we have
  libraries in Chapter 4.

## Get this episode

```bash
git checkout ep03-inc-src-layout
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/hello.exe
```

---
[← Chapter 2: A header and its source file](02-header-source.md) · [Chapter 4: Your first library module →](04-menus-library.md)
