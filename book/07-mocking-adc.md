# Chapter 7 — Mocking a Hardware Leaf

> Unit-test a function that reads hardware — without any hardware — by splitting the module so the test can swap out the one function that touches a register. **Branch:** `ep07-mocking-adc`

## What you'll build

An `adc` module that reads a 12-bit ADC (raw counts `0..4095`) and converts the reading to volts against a 3.0 V reference. The conversion is trivial math — `(raw / 4095) * 3.0` — but you can't run it on your laptop, because reading the ADC means poking a hardware register that only exists on the microcontroller.

So the real lesson isn't the math. It's the *structure* that lets you test the math on your PC with **no hardware at all**, by **mocking** the one function that touches the chip.

Three new source files make up the module:

| File | Role |
|------|------|
| `adc/Inc/adc.h` | Declares both functions: the hardware leaf and the logic. |
| `adc/Src/adc.c` | The **logic** — the conversion math. No hardware access. |
| `adc/Src/adc_hal.c` | The **real hardware read**. Linked into the app, *not* into the test. |

Plus a unit test, `tests/test_adc.c`, that supplies its own fake hardware read and checks the math for four different raw values. By the end you'll have `test_adc.exe` reporting 4 passing tests, the app printing `ADC voltage = 1.500 V`, and `ctest` running all three test suites green.

## Where we left off

In Chapter 6 you stood up host-side unit testing: cmocka as the test framework, `enable_testing()` + `add_test()` to register tests with CTest, and a `tests/` folder building `test_math` and `test_menus`. Running `ctest` from `build/` ran both and reported them passing.

Those tests were easy because `add()` and the menu printers are **pure**: give them inputs, check outputs, no environment involved. This chapter tackles the case that trips everyone up the first time — testing a function whose whole job is to talk to hardware that isn't there.

## The concept

Here's the problem stated plainly. You want to test this (shown with the constants inlined so the math is obvious — the committed `adc.c` factors `4095.0f` and `3.0f` into named `#define`s, which you'll see in full shortly):

```c
float adc_read_voltage(void)
{
    uint16_t raw = adc_read_raw();
    return ((float)raw / 4095.0f) * 3.0f;
}
```

The math is what you care about. But `adc_read_raw()` reads an ADC data register that only exists on the MCU. Compile this for your PC and call it, and there's nothing to read. You can't test the math because it's welded to hardware you don't have.

The fix is a design idea, not a CMake trick: **split the module into two translation units so there's a seam you can cut.**

- `adc.c` holds the **logic**. It calls `adc_read_raw()` but never touches a register itself.
- `adc_hal.c` holds the **real hardware read** — the actual `adc_read_raw()`.

`adc_read_raw()` is a *hardware leaf*: the lowest-level function, the one place the register access lives. Because the logic only ever reaches hardware *through* that one function, if you can replace that function you can feed the logic any value you like.

Now the key move, and it's a **link seam** — a swap that happens at *link time*, not compile time. C's linker resolves each function name (symbol) to exactly one definition. So:

- The **app** links `adc.c` **+** `adc_hal.c`. The only `adc_read_raw()` present is the real one, so that's what gets used.
- The **test** links `adc.c` **+** its own fake `adc_read_raw()` defined inside `test_adc.c`, and **deliberately does not link `adc_hal.c`**. The real one simply isn't in the build, so the linker resolves the call to the test's fake.

```text
APP  build:   adc.c  +  adc_hal.c      → adc_read_raw() = real hardware read (returns 2048)
TEST build:   adc.c  +  test_adc.c     → adc_read_raw() = fake we control
              (adc_hal.c left out)
```

That swap of one symbol at link time **is** the mock. No `#ifdef`, no function pointers, no framework magic — just controlling which object files go into the link. This is the most robust way to mock a hardware leaf in C, and the whole chapter hinges on it.

## Step by step

1. **Write the leaf declaration.** Put `adc_read_raw()` in `adc.h` so both the real code and the mock agree on its signature.
2. **Isolate the logic.** `adc.c` calls `adc_read_raw()` and does the conversion — nothing else.
3. **Isolate the hardware.** `adc_hal.c` holds the real `adc_read_raw()`. On a real chip it would start a conversion and read the data register; here it returns a fixed placeholder so the app links and runs.
4. **Write the fake in the test.** `test_adc.c` defines its *own* `adc_read_raw()` — a "settable fake" backed by a `static` variable the tests write to.
5. **Wire the two build variants in CMake.** The app target links `adc` (both files). The `test_adc` target compiles `adc.c` only — never `adc_hal.c`.
6. **Assert with a tolerance.** The results are floats, so compare with an epsilon rather than expecting exact equality.

### The leaf and the logic

`adc/Inc/adc.h` declares both functions and documents that the leaf is the thing tests replace:

```c
#ifndef ADC_H
#define ADC_H

#include <stdint.h>

/*
 * Hardware leaf: returns one raw 12-bit ADC sample (0..4095).
 * Real implementation lives in adc_hal.c. In unit tests this symbol is
 * REPLACED by a mock so we can feed the logic any value we like.
 */
uint16_t adc_read_raw(void);

/*
 * Reads the ADC and converts the raw count to volts.
 * 12-bit full scale = 4095 counts, reference voltage Vref = 3.0 V.
 * Example: raw 4095 -> 3.0 V, raw 0 -> 0.0 V, raw 2048 -> ~1.5 V.
 */
float adc_read_voltage(void);

#endif /* ADC_H */
```

`adc/Src/adc.c` is the function under test. Notice it contains **no** hardware access — only a call to the leaf and the arithmetic:

```c
#include "adc.h"

#define ADC_MAX_COUNT 4095.0f   /* 12-bit full scale */
#define ADC_VREF      3.0f      /* reference voltage in volts */

/*
 * The function under test. It contains NO hardware access itself -- it only
 * calls adc_read_raw() (the leaf) and does the conversion math. That split is
 * exactly what lets a test mock adc_read_raw() and check this math in isolation.
 */
float adc_read_voltage(void)
{
    uint16_t raw = adc_read_raw();
    return ((float)raw / ADC_MAX_COUNT) * ADC_VREF;
}
```

### The real hardware read

`adc/Src/adc_hal.c` is the leaf's real implementation. Here it's a placeholder returning mid-scale, with a comment showing what real register access looks like:

```c
#include "adc.h"

/*
 * The REAL hardware read. On a real MCU this would start a conversion and read
 * the ADC data register, e.g.:
 *
 *     ADC1->CR |= ADC_CR_ADSTART;
 *     while (!(ADC1->ISR & ADC_ISR_EOC)) { }
 *     return (uint16_t)ADC1->DR;
 *
 * There is no hardware here, so we return a fixed placeholder. This file is
 * linked into the app but NOT into the unit test -- the test supplies its own
 * adc_read_raw() mock instead.
 */
uint16_t adc_read_raw(void)
{
    return 2048u;   /* pretend the pin sits at mid-scale (~1.5 V) */
}
```

### The mock: a "settable fake"

`tests/test_adc.c` defines its own `adc_read_raw()`. This is a **settable fake**: a `static` variable holds the value, the fake returns it, and each test writes the value it wants before calling the logic.

```c
/* Required by cmocka, in this order, before cmocka.h. */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "adc.h"

/*
 * ===== THE MOCK (a "settable fake") =====
 * This is our own definition of adc_read_raw(). Because the test links adc.c
 * but NOT adc_hal.c, THIS definition is the one that gets used at link time --
 * it replaces the real hardware read. This "link seam" is the heart of mocking.
 *
 * The fake simply returns whatever the test put in g_fake_raw. So each test
 * sets the raw ADC value it wants, then checks that adc_read_voltage() does the
 * conversion math correctly -- with no hardware involved.
 *
 * (cmocka also has a will_return()/mock() mechanism, but on this prebuilt
 * cmocka.dll it corrupts the heap when mixed with our gcc build. A settable
 * fake is simpler and robust -- the recommended way to mock a hardware leaf.)
 */
static uint16_t g_fake_raw;

uint16_t adc_read_raw(void)
{
    return g_fake_raw;
}
```

Each test sets `g_fake_raw`, calls `adc_read_voltage()`, and asserts the result. The `#include` order matters to cmocka — the four system headers, in that exact order, come before `cmocka.h`.

The four cases cover the boundaries and a couple of points in between:

```c
/* raw 0 -> 0.0 V */
static void test_voltage_zero(void **state)
{
    (void)state;
    g_fake_raw = 0;
    assert_float_equal(adc_read_voltage(), 0.0f, 0.001f);
}

/* raw 4095 (full scale) -> Vref = 3.0 V */
static void test_voltage_full_scale(void **state)
{
    (void)state;
    g_fake_raw = 4095;
    assert_float_equal(adc_read_voltage(), 3.0f, 0.001f);
}

/* raw 2048 (mid) -> 2048/4095*3 = ~1.5004 V */
static void test_voltage_midscale(void **state)
{
    (void)state;
    g_fake_raw = 2048;
    assert_float_equal(adc_read_voltage(), 1.5004f, 0.001f);
}

/* raw 1365 -> 1365/4095*3 = ~1.0 V */
static void test_voltage_one_volt(void **state)
{
    (void)state;
    g_fake_raw = 1365;
    assert_float_equal(adc_read_voltage(), 1.0f, 0.001f);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_voltage_zero),
        cmocka_unit_test(test_voltage_full_scale),
        cmocka_unit_test(test_voltage_midscale),
        cmocka_unit_test(test_voltage_one_volt),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
```

Notice `test_voltage_midscale` expects `1.5004f`, not `1.5f`. That's the real arithmetic: `2048 / 4095 * 3 = 1.50037…`. The tolerance argument (`0.001f`) is what lets the assertion pass anyway — more on that in [Gotchas](#gotchas).

## The CMake, explained

Three pieces change. First, the new module gets its own `adc/CMakeLists.txt`. The library is the logic **plus** the real hardware read — that's the "app" variant:

```cmake
# --- The "adc" module ---
# Library = the conversion logic (adc.c) + the real hardware read (adc_hal.c).
# The app links both; the unit test links only adc.c and mocks the leaf.
add_library(adc
    Src/adc.c
    Src/adc_hal.c
)

target_include_directories(adc PUBLIC Inc)
```

`add_library(adc ...)` with no `STATIC`/`SHARED` keyword builds a **static** library — the default here — so you'll get `build/adc/libadc.a`. Marking the include directory `PUBLIC` means any target that links `adc` automatically inherits `adc/Inc/` on its header search path; that's why `main.c` can `#include "adc.h"` without the top-level file spelling out the path.

Second, the top-level `CMakeLists.txt` pulls the module in and links it into the app:

```cmake
# Pull in the "adc" module (defines the "adc" library target).
add_subdirectory(adc)
```

```cmake
# Link the "menus" library into the app. Because menus' Inc was marked PUBLIC,
# the app automatically gets menus' header search path too.
target_link_libraries(hello PRIVATE menus adc)
```

The app (`hello`) now links `adc`, which carries both `adc.c` and `adc_hal.c` — the **real** hardware read. This is the app build variant.

Third — and this is where the mock is actually wired — `tests/CMakeLists.txt` builds `test_adc` from `test_adc.c` **plus `adc.c` only**:

```cmake
# --- adc module test (demonstrates MOCKING) ---
# Compile ONLY adc.c (the logic). We deliberately do NOT include adc_hal.c,
# so the mock adc_read_raw() inside test_adc.c is the definition that links.
add_executable(test_adc
    test_adc.c
    ${CMAKE_SOURCE_DIR}/adc/Src/adc.c
)

target_include_directories(test_adc PRIVATE
    ${CMAKE_SOURCE_DIR}/adc/Inc
    ${CMOCKA_DIR}/include
)

target_link_libraries(test_adc PRIVATE ${CMOCKA_DIR}/lib/libcmocka.dll.a)

add_custom_command(TARGET test_adc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMOCKA_DIR}/bin/cmocka.dll $<TARGET_FILE_DIR:test_adc>
)

add_test(NAME adc_tests COMMAND test_adc)
```

Read that source list carefully — it's the whole point of the chapter. The test does **not** link the `adc` library (which would drag in `adc_hal.c`); it names `adc.c` directly and stops there. Because `adc_hal.c` is absent, the only `adc_read_raw()` in the link is the fake in `test_adc.c`. Same `adc.c`, two different companions:

| Target | Sources in the link | Which `adc_read_raw()` wins |
|--------|--------------------|-----------------------------|
| `hello` (app) | `adc.c` + `adc_hal.c` (via `libadc.a`) | real hardware read → `2048` |
| `test_adc` (test) | `adc.c` + `test_adc.c` | fake → `g_fake_raw` |

Everything else here mirrors the cmocka setup from Chapter 6: `CMOCKA_DIR` points at the installed library, `target_include_directories` adds `cmocka.h` (and this module's `adc/Inc/`), `target_link_libraries` links `libcmocka.dll.a`, the `POST_BUILD` custom command copies `cmocka.dll` next to the `.exe` so it runs standalone, and `add_test(NAME adc_tests ...)` registers it with CTest.

## Build and run

Configure, build, then run the test executable directly:

```bash
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/tests/test_adc.exe
```

The test binary reports all four cases passing:

```text
[==========] Running 4 test(s).
[ RUN      ] test_voltage_zero
[       OK ] test_voltage_zero
[ RUN      ] test_voltage_full_scale
[       OK ] test_voltage_full_scale
[ RUN      ] test_voltage_midscale
[       OK ] test_voltage_midscale
[ RUN      ] test_voltage_one_volt
[       OK ] test_voltage_one_volt
[==========] 4 test(s) run.
[  PASSED  ] 4 test(s).
```

Run the app and you'll see it use the **real** HAL — `adc_read_raw()` returns `2048`, and `2048 / 4095 * 3 = 1.5004`, which `%.3f` rounds to `1.500`:

```text
Hello, World!
2 + 3 = 5
ADC voltage = 1.500 V

... menus ...
```

And from inside `build/`, `ctest` runs all three registered suites — the two from Chapter 6 plus the new one:

```text
Test project C:/.../cmake-course/build
    Start 1: math_tests
1/3 Test #1: math_tests .......................   Passed
    Start 2: menus_tests
2/3 Test #2: menus_tests ......................   Passed
    Start 3: adc_tests
3/3 Test #3: adc_tests ........................   Passed

100% tests passed, 0 tests failed out of 3
```

Same `adc.c` conversion logic, exercised two ways: fed fake values by the test, and fed the real placeholder by the app.

## What's autogenerated

Everything below is produced by the build — you never edit or commit any of it:

| Path | What it is |
|------|-----------|
| `build/adc/libadc.a` | The static `adc` library (logic + real HAL), built from `adc/CMakeLists.txt`. |
| `build/tests/test_adc.exe` | The compiled test binary (from `test_adc.c` + `adc.c`, with the fake linked in). |
| `build/tests/cmocka.dll` | Copied next to the `.exe` by the `POST_BUILD` command so the test runs standalone. |

As always, the whole `build/` tree regenerates from your source files — delete it and reconfigure and you get an identical result.

## Gotchas

- **Don't compile `adc_hal.c` into the test.** This is the mistake that breaks the whole technique. If you add `adc_hal.c` to the `test_adc` sources (or link the `adc` library instead of naming `adc.c` directly), you'll have **two** definitions of `adc_read_raw()` — the real one and the fake — and the link fails with a *multiple definition* / duplicate-symbol error. The mock works *precisely because* the real leaf is left out of the link.

- **Floats aren't exact — assert with an epsilon.** `2048 / 4095 * 3` is `1.50037…`, not a clean `1.5`, and even "clean" values accumulate tiny binary-floating-point error. That's why the tests use `assert_float_equal(actual, expected, 0.001f)` — the third argument is a tolerance. A plain equality check (`assert_true(actual == 1.5f)`) would be brittle and could fail on rounding alone. Pick an epsilon that's loose enough to absorb rounding but tight enough to catch real bugs.

- **Why a settable fake instead of cmocka's `will_return()`/`mock()`?** cmocka ships a return-value mechanism (`will_return()` queues a value, `mock()` pops it inside the mock function). It's idiomatic cmocka — but with *this* prebuilt `cmocka.dll` compiled by a different toolchain than our gcc, mixing it in corrupts the heap and the test crashes with exit code `c0000374` (a Windows heap-corruption code). The settable fake sidesteps that entirely, and it's also just a cleaner pattern for a hardware leaf: one variable, one return, nothing to queue. Reach for the settable fake first; save `will_return()` for when you genuinely need per-call scripted return sequences and your cmocka build is known-good.

- **The link seam swaps whole symbols, not per-call behavior.** For the lifetime of the test process, *every* call to `adc_read_raw()` hits the fake. That's exactly what you want for a leaf, but keep it in mind: the fake is process-global state (`g_fake_raw` is `static`), so set it at the top of each test rather than assuming it carries a value from a previous one.

## Recap

- To test a function that touches hardware, **split the module**: put the register access in a **hardware leaf** (`adc_hal.c`) and keep the logic (`adc.c`) hardware-free so it only reaches hardware *through* the leaf.
- **Mocking here is a link seam.** The test links `adc.c` + its own fake `adc_read_raw()` and leaves `adc_hal.c` out; the linker resolves the call to the fake. That one-symbol swap at link time *is* the mock — no framework magic required.
- Two build variants share `adc.c`: the **app** links it with the real `adc_hal.c`, the **test** links it with the fake. CMake wires this by choosing which sources go into each target.
- Use a **settable fake** (`static` variable + trivial return) for hardware leaves — it's robust and dodges the `will_return()` heap-corruption issue with this prebuilt `cmocka.dll`.
- Compare floats with a **tolerance** (`assert_float_equal(..., 0.001f)`), never exact equality.
- Compiling `adc_hal.c` into the test would give a **duplicate-symbol** link error — the mock depends on leaving it out.

## Get this episode

```bash
git checkout ep07-mocking-adc
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
./build/tests/test_adc.exe
```

---
[← Chapter 6: Unit tests with cmocka + CTest](06-cmocka-ctest.md) · [Chapter 8: Build options and #ifdef →](08-build-options.md)
