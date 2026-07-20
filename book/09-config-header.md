# Chapter 9 — A Generated Config Header

> Replace a pile of scattered `-D` flags with **one generated header** that CMake fills in from your build options — using `configure_file()`, an INTERFACE library, and the three `#cmakedefine` syntaxes. **Branch:** `ep09-config-header`

## What you'll build

The same app as before — banner, greeting, an ADC voltage, two menus — but the
five build-time knobs no longer travel as loose `-D` compiler flags. Instead you
write **one template**, `config/app_config.h.in`, and CMake *generates* a real
header, `build/generated/app_config.h`, with every flag filled in. Your C code
just does `#include "app_config.h"`.

```text
cmake-course/
├── CMakeLists.txt              # declares 5 flags + configure_file() + app_config target
├── config/
│   └── app_config.h.in         # NEW: the template you write
├── Src/
│   └── main.c                  # #include "app_config.h"; uses banner/verbose/debug flags
├── adc/
│   ├── CMakeLists.txt          # links app_config PRIVATE
│   └── Src/adc.c               # uses ADC_VREF_MV instead of a hardcoded 3.0f
├── menus/
│   └── CMakeLists.txt          # keeps the conditional source, drops the -D macro
└── build/
    └── generated/
        └── app_config.h        # GENERATED at configure time — never edit this
```

The five flags we manage this way:

| Flag | Kind |
| --- | --- |
| `ENABLE_DEBUG_MENU` | on/off |
| `ENABLE_STARTUP_BANNER` | on/off |
| `VERBOSE_LOGGING` | 0/1 |
| `ADC_VREF_MV` | integer value |
| `APP_VERSION` | string value (from the project version) |

## Where we left off

In Chapter 8 we introduced `option()` and drove one `#ifdef` by handing the
compiler a macro directly. In `menus/CMakeLists.txt` we did both jobs at once:

```cmake
if(ENABLE_DEBUG_MENU)
    # 1) compile the extra source into this library
    target_sources(menus PRIVATE Src/debug_menu.c)

    # 2) define the preprocessor macro ENABLE_DEBUG_MENU
    target_compile_definitions(menus PUBLIC ENABLE_DEBUG_MENU)
endif()
```

And the ADC's reference voltage was frozen in source:

```c
#define ADC_MAX_COUNT 4095.0f   /* 12-bit full scale */
#define ADC_VREF      3.0f      /* reference voltage in volts */
```

That works for *one* flag. But the moment you have five — some on/off, some
numeric, some strings — the `target_compile_definitions(... -D…)` approach gets
noisy: every flag needs its own line, the values are stringly-typed, and there's
no single place a reader can look to see "what is this build configured as?"
This chapter fixes that.

## The concept

### Why not just keep adding `-D` flags?

Each `-D` flag is invisible until you dig through the build system. Ten of them
scattered across three `CMakeLists.txt` files is unmanageable, and numeric or
string values (`-D ADC_VREF_MV=3300`, `-D APP_VERSION=\"1.2.0\"`) are awkward to
pass and easy to get wrong.

The scalable answer is a **generated config header**. You write a *template*
with placeholders, and CMake's `configure_file()` command produces a real header
with the placeholders replaced by the current values. Your C code includes that
header and sees ordinary `#define`s. One file to read, one file that documents
the whole build configuration.

### The template: three substitution syntaxes

A `configure_file()` template (`.h.in` by convention) understands three special
syntaxes. All five of our flags use one of them:

- **`#cmakedefine NAME`** — an *on/off* macro. If the CMake variable `NAME` is
  true, this line becomes `#define NAME`. If it's false, it becomes a
  *commented-out* `/* #undef NAME */`. Test it in C with `#ifdef`.
- **`#cmakedefine01 NAME`** — a *0/1* macro. This line *always* becomes a
  `#define`, either `#define NAME 0` or `#define NAME 1`. Because it is
  **always defined**, you test it with `#if NAME`, **not** `#ifdef`. (More on
  that trap in Gotchas — it's the #1 config-header bug.)
- **`@VAR@`** — a *value substitution*. The token `@VAR@` is replaced verbatim
  with the CMake variable's value. Great for numbers and strings.

## Step by step

### 1. Write the template `config/app_config.h.in`

This is the one file you author by hand. It's normal C with the three special
syntaxes sprinkled in:

```c
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*
 * GENERATED FILE -- do not edit the copy in build/generated/app_config.h.
 * CMake produces it from this template (config/app_config.h.in) via
 * configure_file(). Change the values through CMake options instead, e.g.
 *   cmake -S . -B build -D VERBOSE_LOGGING=ON -D ADC_VREF_MV=3300
 */

/* --- on/off flags ---
 * Each line becomes "#define X" when the CMake variable is ON/true,
 * or a commented-out #undef when OFF/false. Test with #ifdef / #ifndef. */
#cmakedefine ENABLE_DEBUG_MENU
#cmakedefine ENABLE_STARTUP_BANNER

/* --- 0/1 flag ---
 * Always #defined, to 0 or 1. Test with #if (NOT #ifdef, since it is
 * always defined). Handy when you want an "always has a value" flag. */
#cmakedefine01 VERBOSE_LOGGING

/* --- value substitutions ---
 * @VAR@ is replaced with the CMake variable's value verbatim. */
#define ADC_VREF_MV @ADC_VREF_MV@
#define APP_VERSION "@PROJECT_VERSION@"

#endif /* APP_CONFIG_H */
```

Note `APP_VERSION`: the placeholder is `@PROJECT_VERSION@`, and it sits *inside*
the C string quotes — so the substituted result is a proper string literal like
`"1.2.0"`. The quotes are yours; CMake only swaps the token.

### 2. Declare the flags and generate the header (top `CMakeLists.txt`)

The project's version is now part of the `project()` call — that's what feeds
`@PROJECT_VERSION@`:

```cmake
# Project name, VERSION (feeds APP_VERSION below), and language(s).
project(hello VERSION 1.2.0 LANGUAGES C)
```

Then we declare all five flags, generate the header, and expose it via an
INTERFACE library:

```cmake
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
```

### 3. Link `app_config` from everything that needs the header

Three targets include `app_config.h`, so three targets link `app_config`. The
app links it alongside its other libraries:

```cmake
target_link_libraries(hello PRIVATE menus adc app_config)
```

The `adc` library links it privately (only `adc.c` includes it):

```cmake
target_link_libraries(adc PRIVATE app_config)
```

And the `test_adc` test compiles `adc.c` directly, so it needs it too:

```cmake
target_link_libraries(test_adc PRIVATE app_config ${CMOCKA_DIR}/lib/libcmocka.dll.a)
```

### 4. Migrate the code to read the header

`menus/CMakeLists.txt` **keeps** the conditional compile of `debug_menu.c`
(whether to compile a source is still a build-system decision) but **drops** the
`target_compile_definitions` — the matching macro now comes from the generated
header:

```cmake
# Whether to COMPILE debug_menu.c is a build-system decision, so it still keys
# off the CMake option directly. (The matching #ifdef macro that main.c tests
# now comes from the generated app_config.h, not a -D flag here.)
if(ENABLE_DEBUG_MENU)
    target_sources(menus PRIVATE Src/debug_menu.c)
endif()
```

`adc/Src/adc.c` includes the generated header and uses `ADC_VREF_MV` (which is
in *millivolts*, hence the `/ 1000.0f`) instead of the old hardcoded `3.0f`:

```c
#include "adc.h"
#include "app_config.h"        /* generated: provides ADC_VREF_MV */

#define ADC_MAX_COUNT 4095.0f  /* 12-bit full scale */

float adc_read_voltage(void)
{
    uint16_t raw = adc_read_raw();
    return ((float)raw / ADC_MAX_COUNT) * (ADC_VREF_MV / 1000.0f);
}
```

`Src/main.c` now reads all three kinds of flag from the header — note which
preprocessor test goes with which kind:

```c
#include <stdio.h>
#include "app_config.h"        /* generated: all 5 build flags */
#include "math_utils.h"
#include "main_menu.h"
#include "settings_menu.h"
#include "adc.h"

#ifdef ENABLE_DEBUG_MENU
#include "debug_menu.h"
#endif

int main(void)
{
#ifdef ENABLE_STARTUP_BANNER
    printf("==============================\n");
    printf("   Hello App   v%s\n", APP_VERSION);   /* value flag (string) */
    printf("==============================\n");
#endif

    printf("Hello, World!\n");
    printf("2 + 3 = %d\n", add(2, 3));
    printf("ADC voltage = %.3f V\n", adc_read_voltage());

#if VERBOSE_LOGGING                                  /* 0/1 flag -> use #if */
    printf("[verbose] Vref = %d mV\n", ADC_VREF_MV); /* value flag (int) */
#endif
    printf("\n");

    print_main_menu();
    printf("\n");
    print_settings_menu();

#ifdef ENABLE_DEBUG_MENU                             /* on/off flag -> use #ifdef */
    printf("\n");
    print_debug_menu();
#endif
    return 0;
}
```

## The CMake, explained

### `configure_file(input output)`

`configure_file()` reads the template, performs the substitutions, and writes
the result. Its two key arguments here are absolute paths built from CMake's own
variables:

- `${CMAKE_SOURCE_DIR}/config/app_config.h.in` — the template (in your source
  tree, tracked in git).
- `${CMAKE_BINARY_DIR}/generated/app_config.h` — the output (in the build tree,
  *not* tracked in git). `configure_file` creates the `generated/` folder for
  you.

Keeping the output under the build directory is deliberate: it's a build
artifact, so it belongs with the other generated files, not next to your source.

### The `app_config` INTERFACE library

```cmake
add_library(app_config INTERFACE)
target_include_directories(app_config INTERFACE ${CMAKE_BINARY_DIR}/generated)
```

An **INTERFACE** library compiles *no* source of its own — there's no `.c` in
it. It exists purely to carry *usage requirements* to whoever links it. Here the
only requirement it carries is one include directory: the folder holding the
generated header.

So when `hello`, `adc`, or `test_adc` do
`target_link_libraries(... app_config)`, they don't pull in any object code —
they inherit the `-I.../generated` include flag. That's the whole trick: instead
of repeating that include path in three places, you name it once and hand it out
by linking. It's the same "PUBLIC include dirs travel with the library" idea from
Chapter 3, taken to its logical end — a library that is *nothing but* an include
path.

### Flag reference

Here is each flag: its CMake declaration, the template line, what the *default*
build generates, and how to test it in C.

| Flag | CMake declaration (default) | Template syntax | Generated line (default) | Test in C |
| --- | --- | --- | --- | --- |
| `ENABLE_DEBUG_MENU` | `option(... OFF)` | `#cmakedefine ENABLE_DEBUG_MENU` | `/* #undef ENABLE_DEBUG_MENU */` | `#ifdef ENABLE_DEBUG_MENU` |
| `ENABLE_STARTUP_BANNER` | `option(... ON)` | `#cmakedefine ENABLE_STARTUP_BANNER` | `#define ENABLE_STARTUP_BANNER` | `#ifdef ENABLE_STARTUP_BANNER` |
| `VERBOSE_LOGGING` | `option(... OFF)` | `#cmakedefine01 VERBOSE_LOGGING` | `#define VERBOSE_LOGGING 0` | `#if VERBOSE_LOGGING` |
| `ADC_VREF_MV` | `set(... 3000 CACHE STRING ...)` | `#define ADC_VREF_MV @ADC_VREF_MV@` | `#define ADC_VREF_MV 3000` | use as int: `ADC_VREF_MV / 1000.0f` |
| `APP_VERSION` | `project(hello VERSION 1.2.0)` → `PROJECT_VERSION` | `#define APP_VERSION "@PROJECT_VERSION@"` | `#define APP_VERSION "1.2.0"` | use as string: `printf("%s", APP_VERSION)` |

## Build and run

Configure and build the defaults:

```bash
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
```

After configuring, look at what CMake generated. `build/generated/app_config.h`
is the template with every placeholder resolved (the comments are copied over
verbatim — this is a faithful full listing):

```text
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*
 * GENERATED FILE -- do not edit the copy in build/generated/app_config.h.
 * CMake produces it from this template (config/app_config.h.in) via
 * configure_file(). Change the values through CMake options instead, e.g.
 *   cmake -S . -B build -D VERBOSE_LOGGING=ON -D ADC_VREF_MV=3300
 */

/* --- on/off flags ---
 * Each line becomes "#define X" when the CMake variable is ON/true,
 * or a commented-out #undef when OFF/false. Test with #ifdef / #ifndef. */
/* #undef ENABLE_DEBUG_MENU */
#define ENABLE_STARTUP_BANNER

/* --- 0/1 flag ---
 * Always #defined, to 0 or 1. Test with #if (NOT #ifdef, since it is
 * always defined). Handy when you want an "always has a value" flag. */
#define VERBOSE_LOGGING 0

/* --- value substitutions ---
 * @VAR@ is replaced with the CMake variable's value verbatim. */
#define ADC_VREF_MV 3000
#define APP_VERSION "1.2.0"

#endif /* APP_CONFIG_H */
```

Notice how each kind resolved: the OFF `#cmakedefine` became a commented
`/* #undef ... */`, the ON one became a bare `#define`, the `#cmakedefine01`
became `#define VERBOSE_LOGGING 0`, and the `@…@` tokens turned into `3000` and
`"1.2.0"`.

Run the app. Banner is ON, verbose is OFF, debug menu is OFF, and Vref is
3000 mV → `2048 / 4095 * 3.0 = 1.500 V`:

```text
$ ./build/hello.exe
==============================
   Hello App   v1.2.0
==============================
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

Now flip several flags. Because `configure_file` runs at *configure* time, you
must re-run the `cmake -S . -B build …` step (not just `cmake --build`):

```bash
cmake -S . -B build -D ENABLE_DEBUG_MENU=ON -D VERBOSE_LOGGING=ON -D ADC_VREF_MV=3300
cmake --build build
```

CMake regenerates `app_config.h` (now with `#define ENABLE_DEBUG_MENU`,
`#define VERBOSE_LOGGING 1`, and `#define ADC_VREF_MV 3300`). The app changes in
three ways: Vref is now 3.3 V so the ADC reads `2048 / 4095 * 3.3 = 1.650 V`, the
`[verbose]` line appears right after it, and the debug menu is compiled in and
printed at the end:

```text
$ ./build/hello.exe
==============================
   Hello App   v1.2.0
==============================
Hello, World!
2 + 3 = 5
ADC voltage = 1.650 V
[verbose] Vref = 3300 mV

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

## What's autogenerated

`build/generated/app_config.h` is a **generated source file**. You never edit
it — anything you type there is overwritten the next time CMake configures. The
file you own is the template, `config/app_config.h.in`; the values come from
CMake variables. This is the same category of artifact as the
`build/compile_commands.json` you met in Chapter 6: a file CMake writes for you
into the build tree, describing the current configuration. Both live under
`build/` (git-ignored) precisely because they're outputs, not inputs.

If you ever wonder what a flag *actually* resolved to, open the generated header.
It is the single, honest record of how this build directory is configured.

## Gotchas

- **`configure_file` runs at CONFIGURE time, not build time.** Changing a flag
  means re-running the `cmake -S . -B build -D …` step. A plain
  `cmake --build build` will *not* regenerate the header, so your flag change is
  silently ignored until you configure again. (CMake does re-run configure
  automatically if you edit the `.h.in` *template*, since it tracks that as a
  dependency — but a `-D` on the command line only takes effect when you pass it
  to a configure step.)
- **`#cmakedefine` vs `#cmakedefine01` — the #1 config-header bug.** A
  `#cmakedefine01` variable is *always* `#define`d (to `0` or `1`), so
  `#ifdef VERBOSE_LOGGING` is **always true** — even when it's `0`. You must use
  `#if VERBOSE_LOGGING`. Conversely, an on/off `#cmakedefine` produces *no*
  macro at all when off, so `#if ENABLE_DEBUG_MENU` would error/warn on the
  undefined symbol — use `#ifdef` for those. Match the test to the syntax:
  `#cmakedefine` → `#ifdef`, `#cmakedefine01` → `#if`.
- **Tests can couple to config.** `tests/test_adc.c` bakes in the expectation
  that full-scale reads **3.0 V** (`assert_float_equal(adc_read_voltage(), 3.0f,
  …)`) and mid-scale reads ~1.5 V. Those numbers assume `ADC_VREF_MV = 3000`. If
  you configure with `-D ADC_VREF_MV=3300`, the app is correct but `test_adc`
  *fails*, because its hardcoded expectations no longer match. Keep `ADC_VREF_MV`
  at its default when running the tests, or make the test compute its expected
  value from `ADC_VREF_MV` instead of hardcoding it.
- **Keep the quotes for string substitutions.** `@PROJECT_VERSION@` expands to
  `1.2.0` (no quotes). The template writes `"@PROJECT_VERSION@"` so the result is
  a valid C string literal. Drop the quotes and `#define APP_VERSION 1.2.0` is a
  syntax error the moment you `printf("%s", APP_VERSION)`.

## Recap

- Scattered `-D` flags don't scale. A **generated config header** collects the
  whole build configuration into one readable file.
- `configure_file(template output)` fills a `.h.in` template with CMake values
  at configure time.
- Three template syntaxes: `#cmakedefine` (on/off → `#define`/commented
  `#undef`, test with `#ifdef`), `#cmakedefine01` (always `0`/`1`, test with
  `#if`), and `@VAR@` (verbatim value substitution for ints and strings).
- An **INTERFACE library** (`app_config`) carries no code — just the generated
  header's include path — so any target that links it can `#include
  "app_config.h"`. Here `hello`, `adc`, and `test_adc` all link it.
- The generated `app_config.h` is a build artifact (like `compile_commands.json`):
  never edit it; edit the template and re-configure.

## Get this episode

```bash
git checkout ep09-config-header
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/hello.exe
```

---
[← Chapter 8: Build options and #ifdef](08-build-options.md) · [Chapter 10: Library-to-library dependencies →](10-lib-plus-lib.md)
