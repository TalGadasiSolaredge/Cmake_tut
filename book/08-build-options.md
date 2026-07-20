# Chapter 8 — Build Options and #ifdef

> Add a feature that only exists when you ask for it. You'll wire a CMake `option()` all the way through to a C `#ifdef`, so one build flag decides whether a whole screen gets compiled in. **Branch:** `ep08-build-options`

## What you'll build

An optional **debug menu**. By default it isn't there at all — the source file that implements it doesn't even get compiled. Flip one switch at configure time and the same project now builds *with* the debug menu, and `main.c` starts calling it.

Two new files carry the feature:

| File | Purpose |
|------|---------|
| `menus/Inc/debug_menu.h` | Declares `print_debug_menu(void)`. |
| `menus/Src/debug_menu.c` | Prints the `=== Debug Menu ===` screen. |

The interesting part isn't the C — it's the chain that connects a CMake decision to a C preprocessor branch. By the end you'll be able to build the project two ways from the exact same sources, and you'll understand every link in that chain (including the one link that trips up almost everyone the first time: the CMake **cache**).

## Where we left off

Chapter 7 finished with a `menus` library, an `adc` library, and a `hello` app that links both. The app printed a couple of numbers and then two menu screens. The relevant tail of `Src/main.c` looked like this:

```c
    print_main_menu();
    printf("\n");
    print_settings_menu();
    return 0;
```

Everything that got listed in a `CMakeLists.txt` was compiled, unconditionally. That's fine when every file is always wanted. But real firmware often has parts you only want *sometimes* — a debug screen, verbose logging, a factory-test mode — and you don't want that code in the shipping binary. This chapter adds the machinery to include code conditionally.

## The concept

We want a single knob that does two things at once:

1. **Decides whether `debug_menu.c` is compiled at all.**
2. **Tells `main.c` whether it's allowed to call into the debug menu.**

CMake gives you a knob with the `option()` command, and the way that knob reaches your C code is a preprocessor **define** — the same `-DNAME` flag you may have typed on a compiler command line by hand. Here's the full chain, and it's worth reading top to bottom because the rest of the chapter is just filling in each arrow:

```text
option(ENABLE_DEBUG_MENU ...)         # 1. a user-settable ON/OFF knob in CMake
        │
        ▼
if(ENABLE_DEBUG_MENU)                  # 2. CMake asks: is the knob ON?
        │
        ▼
target_compile_definitions(... PUBLIC  # 3. if so, add a define to the build
    ENABLE_DEBUG_MENU)
        │
        ▼
gcc ... -DENABLE_DEBUG_MENU ...        # 4. CMake turns that into a compiler flag
        │
        ▼
#ifdef ENABLE_DEBUG_MENU               # 5. in C, the macro is now defined -> true
```

The key command is step 3, `target_compile_definitions`. **That is the command that turns a CMake decision into a `-D` flag on the compiler.** Without it, the `option()` is just a CMake variable that your C code never hears about.

A quick note on the two forms of define:

- `target_compile_definitions(t PUBLIC ENABLE_DEBUG_MENU)` produces `-DENABLE_DEBUG_MENU`. The macro is *defined* (its value is empty), which is exactly what `#ifdef` tests for. This is what we use here — it's an on/off feature flag.
- `target_compile_definitions(t PUBLIC FOO=8)` produces `-DFOO=8`, which the compiler treats as `#define FOO 8`. Use this form when the code needs an actual *value* (a buffer size, a version number) rather than a yes/no.

## Step by step

1. Get onto the episode branch and put the toolchain on your PATH (see [Get this episode](#get-this-episode)).
2. **Add the knob.** In the top `CMakeLists.txt`, declare `option(ENABLE_DEBUG_MENU ...)` — importantly, *before* `add_subdirectory(menus)`, because the subdirectory is the code that reads the knob.
3. **React to the knob.** In `menus/CMakeLists.txt`, an `if(ENABLE_DEBUG_MENU)` block does two things: compiles the extra source with `target_sources`, and publishes the define with `target_compile_definitions`.
4. **Guard the C.** In `Src/main.c`, wrap both the `#include "debug_menu.h"` and the `print_debug_menu()` call in `#ifdef ENABLE_DEBUG_MENU ... #endif`.
5. **Configure and build twice** — once with the default (OFF) and once with the knob flipped ON — and watch the output change.

## The CMake, explained

### The knob (top `CMakeLists.txt`)

Right after the `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)` line and *before* the menus subdirectory is pulled in, we declare the option:

```cmake
# A user-settable build option. Default OFF. Flip it at configure time with:
#   cmake -S . -B build -D ENABLE_DEBUG_MENU=ON
# The value is remembered in the CMake cache until you change it again.
option(ENABLE_DEBUG_MENU "Include the debug menu screen" OFF)

# Pull in the "menus" subfolder. CMake runs menus/CMakeLists.txt,
# which defines the "menus" library target.
add_subdirectory(menus)
```

`option(<name> "<help text>" <default>)` creates a cache variable that is either `ON` or `OFF`. The help text shows up in CMake's GUI tools and in `cmake -LAH`. The default here is `OFF`, so out of the box the feature is absent.

Order matters: the `option()` **must be declared before** `add_subdirectory(menus)`. CMake reads files top to bottom, so if the option came *after* the subdirectory, then when `menus/CMakeLists.txt` runs its `if(ENABLE_DEBUG_MENU)` the variable wouldn't exist yet and the test would silently be false.

### Reacting to the knob (`menus/CMakeLists.txt`)

The library definition is unchanged; we just append a conditional block at the end:

```cmake
# Only when the option is ON:
if(ENABLE_DEBUG_MENU)
    # 1) compile the extra source into this library
    target_sources(menus PRIVATE Src/debug_menu.c)

    # 2) define the preprocessor macro ENABLE_DEBUG_MENU (this is what turns
    #    -D<name> on for the compiler). PUBLIC means anything linking "menus"
    #    -- like the app -- ALSO gets the macro, so main.c's #ifdef sees it.
    target_compile_definitions(menus PUBLIC ENABLE_DEBUG_MENU)
endif()
```

Two commands, two jobs:

- **`target_sources(menus PRIVATE Src/debug_menu.c)`** adds `debug_menu.c` to the `menus` library — but *only inside the `if`*. When the option is OFF, this line never runs, so `debug_menu.c` is never handed to the compiler. The dead feature costs literally nothing: no object file, no code in the binary.
- **`target_compile_definitions(menus PUBLIC ENABLE_DEBUG_MENU)`** is the link that reaches your C code. It tells CMake to pass `-DENABLE_DEBUG_MENU` when compiling — and because it's `PUBLIC`, to pass it to consumers of `menus` too.

### Why `PUBLIC` here

This is the same "usage requirements" idea you met with include directories in Chapter 4, applied to a define.

The define is attached to the `menus` target. But the code that needs to see it — the `#ifdef` — lives in `main.c`, which is part of the `hello` executable, a *different* target. How does `hello` get the define?

Because `hello` links `menus` (`target_link_libraries(hello PRIVATE menus adc)`), and the define was marked `PUBLIC`. A `PUBLIC` property is used **both** when building `menus` itself **and** when building anything that links against `menus`. So the define propagates to `hello`, and `main.c` compiles with `-DENABLE_DEBUG_MENU` even though nobody wrote that flag in the app's own `CMakeLists.txt`.

Had we written `PRIVATE` instead, `debug_menu.c` would still compile with the macro, but `main.c` would not — its `#ifdef` would stay false, the include and the call would be dropped, and you'd get a linker complaint about an unused-but-compiled function or, more likely, simply no debug menu despite building the file. `PUBLIC` is the correct choice precisely because a *consumer* (the app) needs to make a decision based on this flag.

### Guarding the C (`Src/main.c`)

The C side is deliberately dumb: it just asks "is the macro defined?"

```c
#include <stdio.h>
#include "math_utils.h"
#include "main_menu.h"
#include "settings_menu.h"
#include "adc.h"

#ifdef ENABLE_DEBUG_MENU
#include "debug_menu.h"
#endif

int main(void)
{
    printf("Hello, World!\n");
    printf("2 + 3 = %d\n", add(2, 3));
    printf("ADC voltage = %.3f V\n\n", adc_read_voltage());

    print_main_menu();
    printf("\n");
    print_settings_menu();

#ifdef ENABLE_DEBUG_MENU
    printf("\n");
    print_debug_menu();
#endif
    return 0;
}
```

Note there are **two** guarded regions, and both matter. The first guards the `#include` — without it, when the feature is off you'd be including a header for code that wasn't compiled. The second guards the actual call. If the macro is undefined, the preprocessor deletes both regions before the compiler ever sees them, so `main.c` compiles exactly as it did in Chapter 7.

## Build and run

### Default: the option is OFF

Configure and build the normal way. You don't pass `ENABLE_DEBUG_MENU` at all, so it takes its `option()` default of `OFF`:

```bash
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/hello.exe
```

Output — the app runs up through the Settings menu, and there is **no** debug menu:

```text
Hello, World!
2 + 3 = 5
ADC voltage = 1.500 V

=== Main Menu ===
1) Start
2) Settings
3) Quit

=== Settings ===
1) Volume
2) Brightness
3) Back
```

### Flipped: the option is ON

Now turn the knob on with a `-D` at configure time, then rebuild:

```bash
cmake -S . -B build -D ENABLE_DEBUG_MENU=ON
cmake --build build
./build/hello.exe
```

Two things worth noticing about that first line. It reconfigures the *same* `build/` folder, and it **drops `-G Ninja` and `-D CMAKE_C_COMPILER=gcc`** — we don't need to repeat them because the cache already remembers the generator and compiler from the first configure. (That's the very cache behavior this chapter is about, working in your favor.)

Output — the same as before, plus the debug menu at the end:

```text
Hello, World!
2 + 3 = 5
ADC voltage = 1.500 V

=== Main Menu ===
1) Start
2) Settings
3) Quit

=== Settings ===
1) Volume
2) Brightness
3) Back

=== Debug Menu ===
1) Dump registers
2) Force ADC value
3) Back
```

The `=== Debug Menu ===` block appears because `debug_menu.c` was compiled this time *and* `main.c` saw the macro and kept its call.

## What's autogenerated

Remember `compile_commands.json` from earlier chapters — the file CMake writes listing the exact flags for every source file. It's a great way to *see* the define with your own eyes rather than take it on faith.

With the option **ON**, look at how the relevant files are compiled:

```bash
grep ENABLE_DEBUG_MENU build/compile_commands.json
```

You'll find `-DENABLE_DEBUG_MENU` on the command line for `main.c` and `debug_menu.c` — proof that the CMake `option()` really did become a compiler flag. With the option **OFF**, the flag is absent from every entry (and there's no entry for `debug_menu.c` at all, because it was never compiled). Everything in `build/` is generated and disposable, exactly as in Chapter 1 — this file just happens to be a handy window into what CMake decided.

## Gotchas

- **The cache remembers your option — a plain reconfigure does NOT reset it.** This is the big one. When you run `cmake -S . -B build -D ENABLE_DEBUG_MENU=ON`, the value `ON` is written into `build/CMakeCache.txt` and it **persists**. Running `cmake -S . -B build` again later — with no `-D` — does *not* fall back to the `option()` default of `OFF`; it keeps `ON`, because the cache wins. To actually turn it back off you must do one of:
  - pass the opposite explicitly: `cmake -S . -B build -D ENABLE_DEBUG_MENU=OFF`,
  - edit `build/CMakeCache.txt` (or use a cache editor), or
  - delete the build tree and start clean: `rm -rf build`.

  If you're ever unsure what the cache currently holds, list it:

  ```bash
  cmake -LAH build
  ```

  `-L` lists cache variables, `-A` includes advanced ones, `-H` shows each option's help string — so you'll see `ENABLE_DEBUG_MENU:BOOL=ON` (or `OFF`) with its description right there.

- **Declare `option()` before the subdirectory that uses it.** CMake executes top to bottom. If `add_subdirectory(menus)` runs before the `option()` line, the `if(ENABLE_DEBUG_MENU)` inside evaluates an undefined variable (false) and your feature silently never turns on.

- **`#ifdef` vs `#if`.** Here the macro is a bare on/off flag — when it's on, `-DENABLE_DEBUG_MENU` defines it with *no value*. `#ifdef NAME` tests "is this macro defined at all?", which is exactly right for a flag. Don't reach for `#if ENABLE_DEBUG_MENU` here: `#if` evaluates the macro's *value* as a number, and a valueless macro would break it. `#if` is for defines that carry a number (Chapter 9's `#cmakedefine01`, which forces the macro to be exactly `0` or `1` so `#if` is safe). For a simple present/absent switch like this one, `#ifdef` is the tool.

## Recap

- `option(NAME "help" OFF)` creates a user-settable ON/OFF knob, defaulting off.
- The chain is `option()` → `if()` → `target_compile_definitions()` → `-DNAME` on the compiler → `#ifdef NAME` becomes true in C.
- `target_compile_definitions` is *the* command that turns a CMake decision into a `-D` flag. Use `NAME` for a flag (`-DNAME`) or `NAME=value` for a value (`-DNAME=value` → `#define NAME value`).
- Put both the extra `target_sources` and the define inside `if(ENABLE_DEBUG_MENU)`, so the feature costs nothing when off.
- `PUBLIC` makes the define propagate to consumers: `menus` sets it, and `hello` sees it because `hello` links `menus` — the same usage-requirements idea as include dirs.
- Flip it with `-D ENABLE_DEBUG_MENU=ON`. The value lives in `build/CMakeCache.txt` and **persists** until you set it otherwise or wipe `build/`. Inspect it with `cmake -LAH build`.
- Declare the `option()` *before* the subdirectory that reads it.

## Get this episode

```bash
git checkout ep08-build-options
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/hello.exe
# then try it ON:
cmake -S . -B build -D ENABLE_DEBUG_MENU=ON
cmake --build build
./build/hello.exe
```

---
[← Chapter 7: Mocking a hardware leaf](07-mocking-adc.md) · [Chapter 9: A generated config header →](09-config-header.md)
