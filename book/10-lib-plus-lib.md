# Chapter 10 — Library-to-Library Dependencies

> Promote `math_utils` into its own library module so the app, the `menus`
> library, and the unit test all share one definition of `add()` — and meet
> **transitive linking**, the rule that makes static-library dependencies
> propagate automatically. **Branch:** `ep10-lib-plus-lib`

## What you'll build

The program prints exactly what it did in Chapter 9. Nothing about the *output*
changes here (well — almost nothing; the main menu learns to count its own
options). What changes is the **shape of the project**: `math_utils` stops being
a pair of loose files compiled into whatever needs it, and becomes a real
library module with its own folder, its own `CMakeLists.txt`, and its own
`PUBLIC` include directory.

Once it is a library, three completely different targets can all link the *same*
compiled `math_utils`:

- the app (`hello`) — `main.c` calls `add(2, 3)`
- the `menus` library — `main_menu.c` calls `add(2, 1)` to count its options
- the unit test (`test_math`) — links `math_utils` instead of recompiling it

Here is the layout after this episode. The `math/` folder is new; the old flat
`Inc/math_utils.h` and `Src/math_utils.c` are **gone**:

```text
cmake-course/
├── CMakeLists.txt
├── config/
│   └── app_config.h.in
├── math/                 ← NEW module
│   ├── CMakeLists.txt
│   ├── Inc/
│   │   └── math_utils.h  ← moved here from top-level Inc/
│   └── Src/
│       └── math_utils.c  ← moved here from top-level Src/
├── menus/
│   ├── CMakeLists.txt
│   ├── Inc/
│   └── Src/
├── adc/
│   ├── CMakeLists.txt
│   ├── Inc/
│   └── Src/
├── Src/
│   └── main.c            ← the ONLY source the app compiles directly now
└── tests/
    ├── CMakeLists.txt
    ├── test_math.c
    ├── test_menus.c
    └── test_adc.c
```

This is the finale of the course, and it is where every idea we've built up —
libraries, `PUBLIC`/`PRIVATE`, subdirectories, include directories, tests —
clicks together into the pattern real projects actually use.

## Where we left off

By the end of Chapter 9, the project already had two library modules — `menus`
and `adc` — each in its own folder with its own `add_library` and a `PUBLIC`
include directory. We had a generated config header (`app_config.h`) handed out
by an `INTERFACE` target called `app_config`. And `math_utils` was… still stuck
in the past.

`math_utils.h` sat in a top-level `Inc/`, `math_utils.c` in a top-level `Src/`,
and the code got into the app the old-fashioned way — by listing the `.c` file
directly in `add_executable` and putting `Inc/` on the app's private include
path:

```cmake
# Chapter 9's app target
add_executable(hello
    Src/main.c
    Src/math_utils.c        # ← math_utils compiled straight into the app
)
target_include_directories(hello PRIVATE Inc)   # ← so it finds math_utils.h
```

The unit test did something worse: it **recompiled** the same source into its
own executable:

```cmake
# Chapter 9's test
add_executable(test_math
    test_math.c
    ${CMAKE_SOURCE_DIR}/Src/math_utils.c    # ← second, separate compile of add()
)
```

So `add()` was compiled twice — once for the app, once for the test — from the
same source. It worked, but it means the test wasn't verifying the *exact* object
the app ships. And if `menus` had wanted to call `add()`, it had no clean way to
get at it. `math_utils` was the last module that wasn't really a module. This
chapter fixes that.

## The concept

### Every reusable piece becomes a library

The organizing idea of this whole course, stated once more in its final form:
**if code is used by more than one thing, make it a library.** A library is a
named target you build once and link wherever you need it. `menus` and `adc`
already earn their keep this way. `math_utils` is used by the app, and now by
`menus`, and by a test — three consumers. That is three reasons to promote it.

Promoting it is a fixed, three-line recipe you've now seen enough times to
recognize:

```cmake
add_library(math_utils Src/math_utils.c)
target_include_directories(math_utils PUBLIC Inc)
```

`PUBLIC` on the include directory means: use `Inc/` when compiling `math_utils`
itself, **and** hand `Inc/` to anything that links `math_utils`, so consumers can
`#include "math_utils.h"` without repeating the path. That's the same `PUBLIC`
you met with `menus` in Chapter 4 — it's how a library carries its own headers to
its users.

### A library depending on a library

The new move this episode is one library linking another. `menus` now *uses*
`math_utils`:

```cmake
# in menus/CMakeLists.txt
target_link_libraries(menus PRIVATE math_utils)
```

Why `PRIVATE`? Because `math_utils` is an **internal implementation detail** of
`menus`. Look at where `add()` is called — inside `main_menu.c`, a `.c` file.
The `menus` *headers* (`main_menu.h`, `settings_menu.h`) never mention
`math_utils` at all. So code that merely `#include`s a menus header has no need
to know about `math_utils`. `PRIVATE` says exactly that: "`menus` needs
`math_utils` to build itself, but users of `menus` don't inherit it."

Contrast with `PUBLIC`, which you'd use if `main_menu.h` had a function whose
prototype mentioned a `math_utils` type — then a consumer including that header
*would* need `math_utils`'s include dir, and `PUBLIC` would forward it. It
doesn't here, so `PRIVATE` is correct.

### Transitive linking — the heart of this chapter

Here's the part worth slowing down for. Look at the menus **test**:

```cmake
add_executable(test_menus test_menus.c)
target_link_libraries(test_menus PRIVATE menus ${CMOCKA_DIR}/lib/libcmocka.dll.a)
```

`test_menus` links `menus`. It does **not** name `math_utils` anywhere. Yet
`test_menus.c` calls `print_main_menu()`, which calls `add()` — a symbol that
lives in `math_utils`. How does that link succeed?

The answer is two separate mechanisms that `PRIVATE` governs, and it's important
not to blur them together:

**1. The *link requirement* propagates — always, regardless of PRIVATE.**
`menus` is a **static library**: an archive (`libmenus.a`) of compiled `.o`
files. A static library does *not* absorb the objects of the libraries it
depends on — `libmenus.a` does not contain a copy of `add()`. So inside
`libmenus.a`, the call to `add()` is an **unresolved symbol**. It has to be
satisfied at the moment something is finally linked into an executable.
CMake knows this, so when it builds the link line for `test_menus`, it
automatically appends `math_utils` — even though you only wrote `menus`. This is
how static-library link requirements *propagate* to whoever links the library.
It happens whether the dependency was declared `PRIVATE` or `PUBLIC`, because
it's a physical necessity: the symbol must resolve or the link fails.

**2. The *usage requirements* do NOT propagate — because it's PRIVATE.**
"Usage requirements" are things like include directories and compile
definitions. Because `menus` linked `math_utils` as `PRIVATE`, a consumer of
`menus` does **not** inherit `math_utils`'s `PUBLIC` include directory. And
indeed `test_menus.c` includes `main_menu.h` and `settings_menu.h` but never
`math_utils.h` — it doesn't need to, because it never names `add()` directly.
The include path stays private to `menus`. (Had the dependency been `PUBLIC`,
`test_menus` *would* have gotten `math_utils/Inc` on its include path too.)

**3. And `PRIVATE` still lets `menus` itself use the dependency.** This is the
third face of the same keyword. `PRIVATE` isn't "off" — it's "for me, not my
users." So `main_menu.c` can `#include "math_utils.h"` and call `add()` and
compile cleanly, because when building `menus` *itself*, math_utils's include
dir and its object are both in play.

So the one-line summary to *avoid* is "PRIVATE means it doesn't propagate." Too
crude. The precise version: **the link requirement propagates (static libs force
it); the usage requirements (includes/defines) do not, because it's PRIVATE; and
the library itself gets full use of the dependency either way.** That three-way
distinction is the real lesson of this chapter.

### Why the app still links `math_utils` explicitly

You'll notice the app names `math_utils` directly:

```cmake
target_link_libraries(hello PRIVATE math_utils menus adc app_config)
```

That's not redundant with the transitive story. `main.c` calls `add(2, 3)`
*itself* (`2 + 3 = 5`), so `hello` is a first-class, direct consumer of
`math_utils` — it doesn't rely on `menus` to drag the symbol in. When a target
uses a symbol directly, you list the library that provides it directly. Letting
it arrive only transitively would be fragile and would read as an accident.

### Each library carries its own include directory

One more consequence to spell out. In Chapter 9 the app had this line:

```cmake
target_include_directories(hello PRIVATE Inc)   # so it could find math_utils.h
```

It's **gone** now. The app has *no* `target_include_directories` at all. Why?
Because every header the app needs now comes from a library's `PUBLIC` include
directory, forwarded automatically when the app links that library:

| Header the app includes | Comes from |
| --- | --- |
| `math_utils.h` | `math_utils`'s `PUBLIC Inc` |
| `main_menu.h`, `settings_menu.h` | `menus`'s `PUBLIC Inc` |
| `adc.h` | `adc`'s `PUBLIC Inc` |
| `app_config.h` (generated) | `app_config`'s `INTERFACE` include dir |

The app just links the four targets and every include path comes along for the
ride. This is the payoff of `PUBLIC`: a well-formed library is *self-describing*
— link it and its headers are simply there.

## Step by step

**1. Create the `math/` module folder and move the two files into it.** No edits
to the file *contents* — only their location:

- `Inc/math_utils.h` → `math/Inc/math_utils.h`
- `Src/math_utils.c` → `math/Src/math_utils.c`

The top-level `Inc/` and `Src/math_utils.c` no longer exist (the app's `Src/`
now holds only `main.c`).

**2. Write `math/CMakeLists.txt`** — the same `add_library` +
`target_include_directories(... PUBLIC Inc)` recipe you used for `menus` and
`adc`.

**3. In the top `CMakeLists.txt`**, add `add_subdirectory(math)` **before**
`add_subdirectory(menus)` (menus links math_utils, so math_utils's target must
exist first), drop `Src/math_utils.c` from `add_executable(hello ...)`, delete
the app's `target_include_directories`, and add `math_utils` to the app's
`target_link_libraries`.

**4. In `menus/CMakeLists.txt`**, add
`target_link_libraries(menus PRIVATE math_utils)`.

**5. In `menus/Src/main_menu.c`**, include `math_utils.h` and use `add()` to
compute the option count.

**6. In `tests/CMakeLists.txt`**, change `test_math` to link `math_utils`
instead of recompiling `math_utils.c`, and drop the now-unnecessary include dir.

## The CMake, explained

### The new module: `math/CMakeLists.txt`

```cmake
# --- The "math_utils" module ---
# math_utils is now its own library, so ANY target can link it: the app, the
# menus library, and the unit test all share this one definition of add().
add_library(math_utils
    Src/math_utils.c
)

target_include_directories(math_utils PUBLIC Inc)
```

Paths here are relative to `math/` (the folder holding this `CMakeLists.txt`),
so `Src/math_utils.c` and `Inc` point inside the module. This is byte-for-byte
the same shape as `menus/CMakeLists.txt` and `adc/CMakeLists.txt` — that
sameness is the point. Every module looks like every other module.

The moved files are unchanged. For reference:

`math/Inc/math_utils.h`:

```c
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

/* Returns the sum of a and b. */
int add(int a, int b);

#endif /* MATH_UTILS_H */
```

`math/Src/math_utils.c`:

```c
#include "math_utils.h"

int add(int a, int b)
{
    return a + b;
}
```

### `menus` depends on `math_utils`

`menus/CMakeLists.txt` gains one line (shown in context):

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

# menus' own code calls add() from math_utils. PRIVATE: it's an internal
# implementation detail -- code that uses menus' headers doesn't need it.
target_link_libraries(menus PRIVATE math_utils)

# Whether to COMPILE debug_menu.c is a build-system decision, so it still keys
# off the CMake option directly. (The matching #ifdef macro that main.c tests
# now comes from the generated app_config.h, not a -D flag here.)
if(ENABLE_DEBUG_MENU)
    target_sources(menus PRIVATE Src/debug_menu.c)
endif()
```

And `menus/Src/main_menu.c` now actually *uses* the dependency:

```c
#include <stdio.h>
#include "main_menu.h"
#include "math_utils.h"   /* menus now depends on the math_utils library */

void print_main_menu(void)
{
    /* Demo of a library-to-library dependency: menus calls add() from
       math_utils to compute how many options it shows. */
    int option_count = add(2, 1);

    printf("=== Main Menu (%d options) ===\n", option_count);
    printf("1) Start\n");
    printf("2) Settings\n");
    printf("3) Quit\n");
}
```

`add(2, 1)` returns `3`, so the header line becomes
`=== Main Menu (3 options) ===`. A tiny, contrived use — but a genuine one: the
`add()` symbol truly comes from another library, and that's what makes this a
real library-to-library dependency rather than a diagram of one.

### The top `CMakeLists.txt`

Here is the full file for this episode. The lines that changed from Chapter 9
are called out below it:

```cmake
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
```

What changed:

- **`add_subdirectory(math)` appeared**, positioned *before*
  `add_subdirectory(menus)`. Ordering matters here: `menus/CMakeLists.txt` calls
  `target_link_libraries(menus PRIVATE math_utils)`, and the `math_utils` target
  has to already be defined when CMake processes that line. (More on this in
  Gotchas.)
- **`Src/math_utils.c` is gone from `add_executable`.** The app compiles a single
  file now — `Src/main.c`. Everything else is a library.
- **`target_include_directories(hello PRIVATE Inc)` is deleted.** The app owns no
  include directories anymore; every header arrives via a linked library's
  `PUBLIC`/`INTERFACE` include dir.
- **`math_utils` was added to the link line.** `target_link_libraries(hello
  PRIVATE math_utils menus adc app_config)` — because `main.c` calls `add()`
  directly.

### The tests

`tests/CMakeLists.txt` for `test_math` slims down considerably. Before, it
recompiled `math_utils.c` and pointed an include dir at the old top-level `Inc/`.
Now it just links the library:

```cmake
# --- math module test ---
# Now that math_utils is a library, we LINK it instead of recompiling its
# source. Cleaner, and it tests the exact same object the app uses.
add_executable(test_math test_math.c)

# cmocka.h path only -- math_utils.h comes from the math_utils PUBLIC include dir.
target_include_directories(test_math PRIVATE ${CMOCKA_DIR}/include)

# Link math_utils (the code under test) and cmocka.
target_link_libraries(test_math PRIVATE math_utils ${CMOCKA_DIR}/lib/libcmocka.dll.a)
```

Two benefits, both real:

- **The test exercises the exact same compiled object the app links.** There's
  now **one** compilation of `add()`, in `libmath_utils.a`, shared by app and
  test. Previously the test had its own separate build of the source — if a
  compile flag differed, you could in principle test something subtly unlike what
  shipped. That gap is closed.
- **`test_math` no longer needs a path to `math_utils.h`.** It comes from
  `math_utils`'s `PUBLIC Inc`, forwarded on link. The only include dir the test
  still names is cmocka's.

`test_menus` is the transitive-linking exhibit discussed above — it links
`menus`, names no `math_utils`, and still resolves `add()`:

```cmake
# --- menus module test ---
# menus is already a library target, so we just link it (no source files here).
add_executable(test_menus test_menus.c)

target_include_directories(test_menus PRIVATE ${CMOCKA_DIR}/include)

# Link the menus library (brings its Inc/ headers along, since they're PUBLIC)
# and cmocka.
target_link_libraries(test_menus PRIVATE menus ${CMOCKA_DIR}/lib/libcmocka.dll.a)
```

(`test_adc` is unchanged from Chapter 9 — it still compiles `adc.c` on its own
and mocks the hardware leaf, links `app_config`, and registers with CTest.)

## Build and run

Configure and build exactly as before. From the project root:

```bash
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/hello.exe
```

Expected output (banner on, verbose off, debug menu off — all the defaults):

```text
==============================
   Hello App   v1.2.0
==============================
Hello, World!
2 + 3 = 5
ADC voltage = 1.500 V

=== Main Menu (3 options) ===
1) Start
2) Settings
3) Quit

=== Settings ===
1) Volume
2) Brightness
3) Back
```

The only visible difference from Chapter 9 is `(3 options)` in the main-menu
header — proof that `menus` really is calling `add()` from the `math_utils`
library. `2 + 3 = 5` is the app's own direct call to `add()`; `ADC voltage =
1.500 V` is the `adc` library reading a mid-scale count (2048 of 4095) against
the 3.0 V reference.

Now run the tests. From inside `build/`:

```bash
cd build && ctest
```

```text
Test project C:/Users/.../cmake-course/build
    Start 1: math_tests
1/3 Test #1: math_tests .......................   Passed    0.02 sec
    Start 2: menus_tests
2/3 Test #2: menus_tests ......................   Passed    0.02 sec
    Start 3: adc_tests
3/3 Test #3: adc_tests ........................   Passed    0.02 sec

100% tests passed, 0 tests failed out of 3
```

All three suites pass. `math_tests` now runs against the shared `libmath_utils.a`,
and `menus_tests` passes only *because* transitive linking pulled `math_utils`
onto its link line to resolve `add()`.

## What's autogenerated

The `build/` tree now contains a genuinely modular set of build products. Because
each module lives in its own subdirectory, its static library is generated under
a matching subfolder of `build/`:

```text
build/
├── hello.exe                 ← the app (compiles just Src/main.c)
├── math/
│   └── libmath_utils.a       ← NEW: the math_utils static library
├── menus/
│   └── libmenus.a            ← the menus static library
├── adc/
│   └── libadc.a              ← the adc static library
├── generated/
│   └── app_config.h          ← from configure_file()
├── tests/
│   ├── test_math.exe
│   ├── test_menus.exe
│   ├── test_adc.exe
│   └── cmocka.dll            ← copied next to the tests by a POST_BUILD step
├── compile_commands.json     ← from CMAKE_EXPORT_COMPILE_COMMANDS
├── build.ninja
└── CMakeCache.txt, CMakeFiles/, ...
```

Those `.a` files are **static libraries** — archives of compiled `.o` objects.
`libmath_utils.a` holds exactly one definition of `add()`, and it's this archive
that both `hello.exe` and `test_math.exe` link against. As always, everything
under `build/` is generated and git-ignored; you never edit it by hand.

## Gotchas

- **`add_subdirectory(math)` must come before anything that links
  `math_utils`.** CMake processes your `CMakeLists.txt` top to bottom. When it
  reaches `target_link_libraries(menus PRIVATE math_utils)` (inside the `menus`
  subdirectory), the `math_utils` target has to already exist. Since `menus` is
  pulled in by `add_subdirectory(menus)`, the `add_subdirectory(math)` line must
  appear *above* it — which is exactly why it's placed where it is. Put `math`
  after `menus` and you'll get an error like:

  ```text
  Target "menus" links to target "math_utils" but the target was not found.
  ```

  (Strictly, CMake can sometimes tolerate forward references between top-level
  targets, but relying on that is asking for trouble — declare a module before
  its consumers and the ordering is never in question.)

- **Circular library dependencies are not allowed.** `menus` links `math_utils`.
  If you then made `math_utils` link `menus`, you'd have a cycle — A needs B needs
  A — and CMake will refuse it. Dependencies must form a one-way graph
  (a *DAG*): higher-level modules depend on lower-level ones, never the reverse.
  If two modules genuinely need each other, that's a design smell — usually the
  shared piece wants to be factored into a third, lower-level library that both
  depend on.

- **`math_utils.h` now lives in `math/Inc/` only.** The old top-level `Inc/` is
  gone, and with it the app's `target_include_directories(hello PRIVATE Inc)`.
  The one and only thing that puts `math_utils.h` on a consumer's include path is
  `math_utils`'s `PUBLIC Inc` directory, delivered when you link `math_utils`. If
  you ever see `fatal error: math_utils.h: No such file or directory`, the
  question is no longer "did I add an include dir?" but "did this target link
  `math_utils`?" — linking the library *is* how you get its header.

- **Don't confuse "links `menus`" with "gets math_utils's headers."** Because
  `menus` links `math_utils` as `PRIVATE`, linking `menus` gives you `menus`'s
  headers but **not** `math_utils`'s. `test_menus` proves this works as intended:
  it resolves `add()` at link time (the link requirement propagates) yet never
  gets `math_utils.h` on its include path (the usage requirement doesn't). If you
  *wanted* `math_utils.h` visible to `menus`'s consumers, you'd declare the
  dependency `PUBLIC` — but here you don't, so `PRIVATE` is right.

## Recap

- If code is used by more than one target, it should be a **library**. This
  episode promotes `math_utils` into its own module (`math/`), removing the old
  flat `Inc/math_utils.h` and `Src/math_utils.c`, so the app, `menus`, and the
  unit test all share one compiled definition of `add()`.
- Making a module is a **fixed recipe**: a folder with `add_library(...)` +
  `target_include_directories(... PUBLIC Inc)`, then `add_subdirectory(newmod)`
  and add it to a consumer's `target_link_libraries`. Every module in the project
  now looks identical — that sameness is how large projects stay manageable.
- A library can link another library. `menus` links `math_utils` **PRIVATE**
  because `math_utils` is an internal implementation detail — used in `menus`'s
  `.c`, never exposed by its headers.
- **Transitive linking** has three faces, and they must not be flattened into one
  slogan: (1) the **link requirement propagates** — because static libraries
  don't absorb their dependencies' objects, CMake automatically adds
  `math_utils` to the link line of anything that links `menus`, so `add()`
  resolves; (2) the **usage requirements do not propagate** under `PRIVATE` — a
  consumer of `menus` does not inherit `math_utils`'s include dir; (3) `menus`
  **itself** still enjoys full use of `math_utils` when compiling its own sources.
- Each library carries its **own `PUBLIC` include directory** to consumers, which
  is why the app dropped `target_include_directories` entirely — linking a library
  brings its headers along for free.
- The app links `math_utils` **directly** because `main.c` calls `add()` itself;
  a symbol you use directly should come from a library you name directly, not one
  you happen to get transitively.

## Get this episode

```bash
git checkout ep10-lib-plus-lib
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/hello.exe
cd build && ctest
```

And that's the course. You started in Chapter 1 with a single `add_executable`
and a "Hello, World!", and you've arrived at a properly modular project: several
library modules that each own their sources and headers, a generated config
header driven by build options, a real unit-test suite wired into CTest, and
libraries that depend on other libraries with their link and usage requirements
propagating correctly. That is genuinely how professional C and C++ codebases are
structured — you now have the whole shape, not a toy version of it. Congratulations
on finishing. When you need to look a command up again, the appendices are your
quick reference: **Appendix A** collects every CMake command you met, and the
later appendices cover the compiler-flag and testing recipes. Go build something.

---
[← Chapter 9: A generated config header](09-config-header.md) · [Appendix A: Command reference →](appendix-a-commands.md)
