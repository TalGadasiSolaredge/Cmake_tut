# Chapter 4 — Your First Library Module

> Split a self-contained feature into its own folder that builds as a **library**, then link it into the app — the pattern every large CMake project is built from. **Branch:** `ep04-menus-library`

## What you'll build

So far our project has been one executable, `hello`, compiled from a flat list of
source files in the top-level `CMakeLists.txt`. That works when you have three
files. It stops working when you have three hundred.

In this episode we carve out a new feature — a couple of on-screen menus — and
give it its own home: a `menus/` folder with its **own** `CMakeLists.txt`. That
folder describes how to build itself, producing a small **static library**. The
top-level file just says "go build the `menus` folder" and "link it into the
app." Crucially, the top-level file never lists a single menu source file and
never mentions the menu headers' folder — and yet `main.c` can include and call
the menu functions. Understanding *why* that works is the whole point of the
chapter.

When you run it, the program prints the greeting and math from before, then two
menus:

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

## Where we left off

At the end of Chapter 3 the project used an `Inc/` + `Src/` layout: headers in
`Inc/`, sources in `Src/`, and one `target_include_directories(hello PRIVATE Inc)`
so `#include "math_utils.h"` resolved. The tree looked like this:

```text
cmake-course/
├── CMakeLists.txt
├── Inc/
│   └── math_utils.h
└── Src/
    ├── main.c
    └── math_utils.c
```

Everything — every source file, every include path — was declared in the single
top-level `CMakeLists.txt`. That is exactly the thing that doesn't scale, and
what we fix now.

## The concept

Three ideas come together in this chapter.

**1. A library is just "compiled code you don't run directly."** Up to now we
built an *executable* with `add_executable` — something with a `main()` you can
run. A **library** is compiled object code with no `main()`, meant to be *linked
into* an executable (or another library). You build one with `add_library`.
By default CMake makes a **static** library: an archive of `.o` object files
that gets copied into the final executable at link time. Our `menus` folder
becomes such a library.

**2. Each folder owns its own build description.** A subfolder can have its own
`CMakeLists.txt`. The parent pulls it in with `add_subdirectory(menus)`, and
CMake runs that child file as part of configuring the project. The top-level
file therefore does **not** need to know the menu source files exist — the
`menus/CMakeLists.txt` lists them. This is the scaling pattern: big projects
stay manageable by **dividing** responsibility across folders, not by growing
one giant file. Each module is self-contained and could, in principle, be
dropped into another project.

**3. Usage requirements: PUBLIC / PRIVATE / INTERFACE.** When a target declares
an include directory (or a compile definition, or a linked library), CMake lets
it say *who else needs it*. This is the single most important idea in modern
CMake. We'll declare the `menus` include folder **PUBLIC**, which means "I need
this to build myself, **and** anything that links me inherits it automatically."
That inheritance is why `main.c` can `#include "main_menu.h"` even though the
top-level `CMakeLists.txt` never mentions `menus/Inc`.

## Step by step

The new folder is self-contained: its own headers under `Inc/`, its own sources
under `Src/`, and its own `CMakeLists.txt`. The full project now looks like
this:

```text
cmake-course/
├── CMakeLists.txt          # top level: builds the app, pulls in menus/
├── Inc/
│   └── math_utils.h        # the app's own headers  (PRIVATE to hello)
├── Src/
│   ├── main.c
│   └── math_utils.c
└── menus/                  # a self-contained module → becomes a library
    ├── CMakeLists.txt      # describes how to build the menus library
    ├── Inc/
    │   ├── main_menu.h      # menus' public headers (PUBLIC → hello inherits)
    │   └── settings_menu.h
    └── Src/
        ├── main_menu.c
        └── settings_menu.c
```

Notice there are now **two** `Inc/` folders. The top-level `Inc/` holds
`math_utils.h`, reachable because `hello` sets `target_include_directories(hello
PRIVATE Inc)` on itself. The `menus/Inc/` holds the menu headers — and `main.c`
reaches *those* only because linking `menus` drags in its PUBLIC include path.
Keep the two straight; the PUBLIC lesson lives in the difference.

### The menu headers

Each header just declares one function. Standard include guards, nothing more.

`menus/Inc/main_menu.h`:

```c
#ifndef MAIN_MENU_H
#define MAIN_MENU_H

/* Prints the main menu to stdout. */
void print_main_menu(void);

#endif /* MAIN_MENU_H */
```

`menus/Inc/settings_menu.h`:

```c
#ifndef SETTINGS_MENU_H
#define SETTINGS_MENU_H

/* Prints the settings menu to stdout. */
void print_settings_menu(void);

#endif /* SETTINGS_MENU_H */
```

### The menu sources

Each source includes its own header and implements the one function.

`menus/Src/main_menu.c`:

```c
#include <stdio.h>
#include "main_menu.h"

void print_main_menu(void)
{
    printf("=== Main Menu ===\n");
    printf("1) Start\n");
    printf("2) Settings\n");
    printf("3) Quit\n");
}
```

`menus/Src/settings_menu.c`:

```c
#include <stdio.h>
#include "settings_menu.h"

void print_settings_menu(void)
{
    printf("=== Settings ===\n");
    printf("1) Volume\n");
    printf("2) Brightness\n");
    printf("3) Back\n");
}
```

### The module's own CMakeLists.txt

This is the new file that makes `menus/` a library. It lives at
`menus/CMakeLists.txt`:

```cmake
# --- The "menus" module ---
# Bundle this folder's sources into a library called "menus".
add_library(menus
    Src/main_menu.c
    Src/settings_menu.c
)

# PUBLIC means: this include dir is used both when building "menus" itself
# AND automatically added to anything that links against "menus".
# So the main app can #include "main_menu.h" without repeating this path.
target_include_directories(menus PUBLIC Inc)
```

Two things worth noticing. The paths (`Src/main_menu.c`, `Inc`) are relative to
**this** file's folder, `menus/` — each `CMakeLists.txt` thinks in terms of its
own directory. And only the `.c` files are listed; the headers are nowhere in
`add_library` (more on that in Gotchas).

### Wiring it into the app

Finally the top-level `CMakeLists.txt` grows three things: it pulls in the
subfolder, and it links the resulting library into `hello`.

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

### Calling the menus from main

`Src/main.c` now includes the menu headers and calls the two functions:

```c
#include <stdio.h>
#include "math_utils.h"
#include "main_menu.h"
#include "settings_menu.h"

int main(void)
{
    printf("Hello, World!\n");
    printf("2 + 3 = %d\n\n", add(2, 3));

    print_main_menu();
    printf("\n");
    print_settings_menu();
    return 0;
}
```

Look at those two includes: `main_menu.h` and `settings_menu.h`. They live in
`menus/Inc/`. The top-level `CMakeLists.txt` never says `menus/Inc` anywhere.
The include still resolves — that's PUBLIC doing its job.

## The CMake, explained

### `add_library` vs `add_executable`

```cmake
add_library(menus
    Src/main_menu.c
    Src/settings_menu.c
)
```

Same shape as `add_executable`: a target name followed by source files. The
difference is the *product*. `add_executable(hello ...)` produces a runnable
program; `add_library(menus ...)` produces a library — here a static archive,
because we didn't ask for anything else. There's no `main()` in the menu
sources, and that's fine: libraries aren't run, they're linked.

### `add_subdirectory(menus)` — the scaling move

```cmake
add_subdirectory(menus)
```

This tells CMake: "there is another `CMakeLists.txt` in the `menus` folder — go
run it." When CMake configures the project, it descends into `menus/`, executes
`menus/CMakeLists.txt`, and comes back knowing about the `menus` target it
defined there.

This is why the top-level file stays short. It does **not** list
`main_menu.c` or `settings_menu.c` — the menus folder owns that knowledge. Add a
tenth source file to the module later and only `menus/CMakeLists.txt` changes;
the top level never notices. Multiply that across dozens of modules and you see
the pattern: you scale a build by **dividing** it into folders that each
describe themselves, not by piling everything into one file.

### PUBLIC vs PRIVATE vs INTERFACE — usage requirements

```cmake
target_include_directories(menus PUBLIC Inc)
```

The keyword before the path answers one question: *besides me, who else needs
this?* CMake calls these **usage requirements**. Here's the whole model:

| Keyword | Do **I** (the target) need it to build? | Do my **consumers** (who link me) inherit it? | Typical use |
|-----------|:--------------------------------------:|:---------------------------------------------:|-------------|
| `PRIVATE` | Yes | No | An implementation detail I use internally but don't expose in my headers. |
| `PUBLIC` | Yes | Yes | Something I use **and** that appears in my public headers, so consumers need it too. |
| `INTERFACE` | No | Yes | I don't compile it myself but consumers must have it — e.g. a header-only library. |

Our `menus` library marks its `Inc` folder **PUBLIC** because two different
parties need that path:

1. **`menus` itself** — `main_menu.c` does `#include "main_menu.h"`, and that
   header is in `Inc`. So `menus` needs `Inc` to compile.
2. **Anything that links `menus`** — `hello` links it and its `main.c` includes
   `main_menu.h` too. Because the path is PUBLIC, CMake **propagates** it: when
   you write `target_link_libraries(hello PRIVATE menus)`, `hello` silently
   inherits `menus/Inc` on its own include path.

Compare with `hello`'s own line, `target_include_directories(hello PRIVATE Inc)`.
That is PRIVATE because nothing links `hello` — it's the top of the chain, so
there are no consumers to inherit anything. `hello` needs `Inc` (for
`math_utils.h`) and no one else does.

If we had marked the menus include dir PRIVATE instead, `menus` would still
build fine on its own — but `hello` would **not** inherit `menus/Inc`, and
`#include "main_menu.h"` in `main.c` would fail with "file not found." The
single word PUBLIC is what makes the module usable by its consumers.

### Only `.c` files go in `add_library`

The `add_library(menus ...)` call lists `main_menu.c` and `settings_menu.c` and
nothing else. Headers are **not** listed there and don't need to be. The
compiler finds headers through the include path (the `Inc` folder we set with
`target_include_directories`), not through the source list. Listing a `.h` in
`add_library` doesn't cause it to be compiled or otherwise "activate" it — it's
inert (see Gotchas).

## Build and run

Same two-step flow as always — configure, then build:

```bash
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/hello.exe
```

The configure step now processes two `CMakeLists.txt` files (top level, then
`menus/` via `add_subdirectory`). The build step compiles the menu sources into
the `menus` library, compiles the app sources, and links the library into
`hello`. Running it prints:

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

The two blank lines are deliberate: the `\n\n` after `2 + 3 = 5` separates the
math from the first menu, and the `printf("\n")` between the calls separates the
two menus.

## What's autogenerated

Because `menus/` is now a subdirectory with its own target, CMake mirrors that
structure inside `build/`. Alongside the `hello.exe` executable, the build now
produces a static library:

```text
build/
├── hello.exe            # the app
└── menus/
    └── libmenus.a       # the static library built from the menus module
```

`libmenus.a` is the compiled `menus` library — an archive of the object files
for `main_menu.c` and `settings_menu.c`. The `lib` prefix and `.a` extension are
the standard naming for a static library with gcc/MinGW; CMake derives that name
from the target name `menus` automatically — you never spell out `libmenus.a`
anywhere. At link time its contents are folded into `hello.exe`, which is why the
running program has the menu code even though you launched only the executable.

Everything else in `build/` — the Ninja files, the `CMakeCache.txt`, the
`CMakeFiles/` bookkeeping — is generated exactly as in earlier chapters. As
always, `build/` is disposable: delete it and re-run the two commands to
regenerate it from scratch.

## Gotchas

- **`add_subdirectory` must come before you use the target it defines.** CMake
  reads the file top to bottom. `add_subdirectory(menus)` is what creates the
  `menus` target, so it has to appear **above** the `target_link_libraries(hello
  PRIVATE menus)` that references it. Put the link line first and CMake errors
  out with something like "Cannot specify link libraries for target 'hello'
  which is not built by this project" / unknown target `menus`.

- **Putting headers in `add_library` does nothing useful.** Adding
  `Inc/main_menu.h` to the `add_library(menus ...)` list won't make the header
  "available" to anyone — sources find headers through the *include path*, not
  the source list. Set the folder with `target_include_directories` (as we did)
  and leave headers out of `add_library`.

- **Forgetting `target_link_libraries` → undefined references.** If you
  `add_subdirectory(menus)` but never link it into `hello`, the compile step
  still succeeds — the headers are found, so `main.c` compiles — but the **link**
  step fails with `undefined reference to 'print_main_menu'` (and
  `print_settings_menu`). The declarations were visible; the actual compiled code
  was never pulled in. The fix is the `target_link_libraries(hello PRIVATE menus)`
  line.

- **Include dir PRIVATE instead of PUBLIC → header not found.** As covered
  above: mark `menus`'s include dir PRIVATE and `hello` won't inherit it, so
  `#include "main_menu.h"` in `main.c` fails. PUBLIC is what propagates the path
  to consumers.

- **Relative paths are relative to each file's own folder.** Inside
  `menus/CMakeLists.txt`, `Src/main_menu.c` and `Inc` mean
  `menus/Src/main_menu.c` and `menus/Inc`. Don't write the paths as if you were
  in the top-level file.

## Recap

- `add_library` builds a **library** (a static archive by default) rather than a
  runnable executable — compiled code meant to be linked into something else.
- `add_subdirectory(menus)` runs `menus/CMakeLists.txt`. The module owns its own
  build description; the top-level file never lists the menu sources. This is how
  builds scale — by **dividing** into self-describing folders.
- Usage requirements (**PRIVATE / PUBLIC / INTERFACE**) control who inherits an
  include path. `menus` marked its `Inc` **PUBLIC**, so linking `menus` gives
  `hello` that header path for free — which is why `main.c` includes
  `main_menu.h` with no mention of `menus/Inc` at the top level.
- Only `.c` files go in `add_library`; headers are found via the include
  directory, not the source list.
- The build now also emits the static library `build/menus/libmenus.a`.

## Get this episode

```bash
git checkout ep04-menus-library
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/hello.exe
```

---
[← Chapter 3: Organizing into Inc/ and Src/](03-inc-src-layout.md) · [Chapter 5: Generators and commands →](05-generators-commands.md)
