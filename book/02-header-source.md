# Chapter 2 — A Header and Its Source File

> Split a program into a header (the promise) and a source file (the code), and tell CMake about the new `.c`. **Branch:** `ep02-header-source`

## What you'll build

The same `hello` program as Chapter 1, but now it does a little arithmetic
through a separate module. We add a tiny `math_utils` "library" — really just one
function, `add` — living in its own pair of files:

- `math_utils.h` — the **declaration**: a one-line promise that a function called
  `add` exists.
- `math_utils.c` — the **definition**: the actual body of `add`.

`main.c` will `#include "math_utils.h"` and print `add(2, 3)`. On the CMake side
you'll learn the single most common day-to-day edit: adding a `.c` file to
`add_executable`.

## Where we left off

Chapter 1 built the smallest possible CMake project. The whole thing was two
files. The `CMakeLists.txt` was:

```cmake
# Minimum CMake version required to process this file.
cmake_minimum_required(VERSION 3.15)

# Project name and the language(s) it uses.
project(hello LANGUAGES C)

# Build an executable called "hello" from main.c.
add_executable(hello main.c)
```

and `main.c` just printed a greeting:

```c
#include <stdio.h>

int main(void)
{
    printf("Hello, World!\n");
    return 0;
}
```

One source file, one executable, no moving parts. Now we split logic out of
`main.c` into its own module — which is where headers enter the story.

## The concept

Real programs are not one giant file. You break them into modules so each piece
can be understood, tested, and reused on its own. In C, a module is
conventionally a **pair** of files:

- A **header** (`.h`) — the *declaration*. It says *what* exists: the name of a
  function, what arguments it takes, what it returns. It is a promise, not an
  implementation.
- A **source** (`.c`) — the *definition*. It contains the actual code that
  fulfills the promise.

**Declaration vs. definition.** This distinction is the heart of the chapter.

- A *declaration* tells the compiler the shape of something so it knows how to
  call it: `int add(int a, int b);`. No body, just a signature ending in a
  semicolon.
- A *definition* is the real thing, with a body:
  ```c
  int add(int a, int b)
  {
      return a + b;
  }
  ```

**Why headers exist.** When the compiler processes `main.c`, it compiles that
file *on its own*. It never sees the inside of `math_utils.c`. So how does it
know that `add` takes two `int`s and returns an `int`? Because `main.c` includes
`math_utils.h`, and the header carries the declaration. The header lets one file
learn how to *call* a function without ever seeing its *body*. The body gets
connected later, by the linker.

That is the mental model to hold onto:

```text
math_utils.h   →  the promise   (declaration)   →  included by anyone who calls add
math_utils.c   →  the code      (definition)     →  compiled once, linked in
main.c         →  the caller    (includes the promise, calls add)
```

**Include guards.** A header often gets included more than once in a single
compilation — directly and again indirectly through some other header. If the
compiler pasted the same declarations in twice, you'd get "redefinition" errors.
The classic fix is the **include guard**:

```c
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

/* ... the header's contents ... */

#endif /* MATH_UTILS_H */
```

Read it as: "if the symbol `MATH_UTILS_H` is *not* already defined, define it and
process everything down to `#endif`." The first time the file is included, the
symbol doesn't exist, so the body is used and the symbol gets defined. Every
later time, the symbol is already defined, so the preprocessor skips straight to
`#endif`. The result: the header's contents appear exactly once, no matter how
many times it's included.

## Step by step

**1. Create the header — the promise.** This is `math_utils.h`:

```c
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

/* Returns the sum of a and b. */
int add(int a, int b);

#endif /* MATH_UTILS_H */
```

Note there's no body — just the declaration `int add(int a, int b);` wrapped in
an include guard. Anyone who includes this file now knows how to call `add`.

**2. Create the source — the code.** This is `math_utils.c`:

```c
#include "math_utils.h"

int add(int a, int b)
{
    return a + b;
}
```

The source file includes its *own* header. That's deliberate: it lets the
compiler check that the definition here matches the declaration in the header
(same name, same parameters, same return type). If they ever drift apart, you
get a compile error instead of a subtle bug.

**3. Call it from `main.c`.** We include the header and use `add`:

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

Two things changed from Chapter 1: the new `#include "math_utils.h"` and the new
`printf` line that calls `add(2, 3)`. Note the quotes in `#include "math_utils.h"`
— quotes are for *your* headers (looked up starting in the file's own directory),
while angle brackets like `#include <stdio.h>` are for system/library headers.

**4. Tell CMake about the new `.c`.** More on this next, but the edit is a single
line in `CMakeLists.txt`: add `math_utils.c` to `add_executable`.

## The CMake, explained

Here is the complete `CMakeLists.txt` for this episode:

```cmake
# Minimum CMake version required to process this file.
cmake_minimum_required(VERSION 3.15)

# Project name and the language(s) it uses.
project(hello LANGUAGES C)

# Build an executable called "hello" from these source files.
# Add every .c file here. Headers (.h) do NOT go in this list.
add_executable(hello main.c math_utils.c)
```

The only change from Chapter 1 is the last line: `add_executable(hello main.c)`
became `add_executable(hello main.c math_utils.c)`.

**This is the key CMake lesson of the chapter:** in `add_executable`, you list
**only the `.c` files** — every source file that needs to be compiled. You do
**not** list `.h` files. Ever.

Why? Because headers are not compiled on their own. They get pulled into a
compilation automatically wherever a `.c` file says `#include`. When
`math_utils.c` and `main.c` each `#include "math_utils.h"`, the preprocessor
splices the header's text into those files before compilation. CMake doesn't
need to be told about the header — the `#include` directives handle it.

Listing a `.h` file in `add_executable` is simply pointless: it doesn't cause the
header to be compiled (headers aren't compilation units), and it doesn't create
any dependency you don't already get from the `#include`. It's a no-op that only
adds noise. So the rule is blunt and easy to remember:

> `add_executable` gets `.c` files, one per module. Headers travel via
> `#include`, never in this list.

**What about finding the header?** At this stage every file lives in the project
root, side by side. When the compiler processes `main.c` and hits
`#include "math_utils.h"`, it looks in the file's own directory first — and finds
it, because they're in the same folder. So we need **no include-path setting yet**.
That changes in Chapter 3, when we move headers into an `inc/` folder and have to
tell the compiler where to look.

## Build and run

Same two steps as always — configure, then build:

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

If you had already configured this project in Chapter 1, you don't have to
re-run the `cmake -S . -B build ...` line by hand. CMake tracks its own input
files, so the next `cmake --build build` notices that `CMakeLists.txt` changed,
re-generates the build files automatically, then compiles. Either way you land
on the same result.

## What's autogenerated

Nothing new compared to Chapter 1. As before, everything CMake produces lands in
the `build/` directory — the generated Ninja files, the object files, and the
final `hello.exe`. It's all disposable: delete `build/` and a fresh
`cmake -S . -B build ...` regenerates it from scratch. That's exactly why
`build/` is in `.gitignore` and never committed. Adding a second source file
didn't introduce any new *kind* of generated output — just one more object file
inside `build/`.

## Gotchas

- **Forgetting to add the new `.c` to `add_executable`.** This is the classic
  first mistake. If you add `math_utils.c` to the project but leave
  `add_executable(hello main.c)` unchanged, the code *compiles* — `main.c` sees
  the declaration from the header, so the compiler is happy — but it fails at the
  **link** step with something like `undefined reference to 'add'`. The linker is
  telling you: someone promised `add` exists, but I was never given the file that
  defines it. The fix is to list `math_utils.c` in `add_executable`.
- **Putting a `.h` file in `add_executable`.** Harmless but pointless. Writing
  `add_executable(hello main.c math_utils.c math_utils.h)` doesn't compile the
  header (headers aren't compilation units) and adds no dependency you didn't
  already have from `#include`. Leave headers out.
- **Missing or mismatched include guards.** If you forget the
  `#ifndef / #define / #endif` guard and the header ends up included twice in one
  file, you'll hit "redefinition" errors. Every header should have a guard whose
  macro name is unique to that file (here, `MATH_UTILS_H`).
- **Angle brackets vs. quotes.** Use `#include "math_utils.h"` (quotes) for your
  own project headers and `#include <stdio.h>` (angle brackets) for system
  headers. Getting these backwards can make the compiler fail to find your header.

## Recap

- A C module is a **pair**: a header (`.h`) holding the **declaration** — the
  promise that a function exists — and a source (`.c`) holding the **definition** —
  the actual code.
- Headers exist so one file can learn how to *call* a function without seeing its
  *body*; the body is connected later by the linker.
- **Include guards** (`#ifndef / #define / #endif`) stop a header's contents from
  being pasted in more than once.
- In `add_executable`, list **only `.c` files** — one per module. **Never** list
  `.h` files; headers are pulled in automatically through `#include`, and listing
  them does nothing.
- Forgetting to add a new `.c` to `add_executable` compiles fine but fails to
  link with `undefined reference`.
- With all files in the project root, the compiler finds `math_utils.h`
  automatically — no include-path setting needed yet.

## Get this episode

```bash
git checkout ep02-header-source
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/hello.exe
```

---
[← Chapter 1: Minimal CMake Hello World](01-hello.md) · [Chapter 3: Organizing into Inc/ and Src/ →](03-inc-src-layout.md)
