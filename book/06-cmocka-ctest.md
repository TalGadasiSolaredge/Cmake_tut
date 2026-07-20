# Chapter 6 — Unit Tests with cmocka + CTest

> Add a `tests/` folder with two cmocka test programs, register them with CTest, and learn the crucial difference between the test *framework* and the test *runner*. **Branch:** `ep06-cmocka-ctest`

## What you'll build

Up to now the project has been code you run. In this chapter we add code that
*checks the other code* — automated unit tests. We create a new `tests/` folder
holding two small test programs:

- `tests/test_math.c` — real assertions against `add()` from our `math_utils`
  module: does `add(2, 3)` really return `5`?
- `tests/test_menus.c` — *smoke tests* for the `menus` library: the menu
  functions only print and return nothing, so there's nothing to assert; we just
  call them and pass if they don't crash.

On the CMake side you'll add three lines to the top `CMakeLists.txt`
(`set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`, `enable_testing()`,
`add_subdirectory(tests)`), write a `tests/CMakeLists.txt` that builds each test
into its own executable and registers it with CTest, and add a tiny
`.vscode/settings.json` so VS Code stops complaining about `#include <cmocka.h>`.

By the end you'll be able to run `ctest` from the `build/` directory and see:

```text
100% tests passed, 0 tests failed out of 2
```

## Where we left off

Chapter 5 didn't change any code — it explained what a *generator* is (Ninja vs.
Visual Studio) and shipped a `README.md` plus a `COMMANDS.md` command reference.
The project itself was still the one we grew across Chapters 1–4: an app called
`hello` built from `Src/`, its headers in `Inc/`, and a small `menus` library
pulled in with `add_subdirectory(menus)`. The top `CMakeLists.txt` ended like
this:

```cmake
# Build an executable called "hello" from these source files (all under Src/).
add_executable(hello
    Src/main.c
    Src/math_utils.c
)

# Tell the compiler where to find this project's own headers (Inc/).
target_include_directories(hello PRIVATE Inc)

# Link the "menus" library into the app.
target_link_libraries(hello PRIVATE menus)
```

We have working code and a way to build it. What we've never had is a way to
*prove* the code is correct — and to keep proving it as the project grows. That's
what testing is for.

## The concept

The single most important idea in this chapter is that **two separate tools** are
at work, and beginners constantly confuse them. Keep them apart in your head:

**cmocka is the test FRAMEWORK.** It lives *inside* each test `.c` file. It gives
you the machinery to write a test: assertion macros like `assert_int_equal`, a
way to declare a test function (`cmocka_unit_test`), and a runner that executes a
group of them (`cmocka_run_group_tests`). When you compile a test file, cmocka
gets baked into the resulting `.exe`. Running that `.exe` executes the tests and
reports the result through its **exit code**: `0` means every assertion passed,
non-zero means at least one failed.

**CTest is the test RUNNER** that ships with CMake — you already have it. CTest
knows *nothing* about cmocka. It doesn't understand assertions, it can't see
individual test functions, it has never heard the word `assert_int_equal`. All it
does is launch each program you registered with `add_test(...)`, wait for it to
finish, and check the exit code: `0` = pass, anything else = fail. Then it tallies
the results.

An analogy that sticks:

> **cmocka is the exam** — the actual questions a student answers inside the
> test. **CTest is the teacher** — who hands out the exams, collects them, and
> tallies who passed. The teacher never reads the questions; they only look at
> the final grade (the exit code).

This split explains a number that surprises everyone the first time. Each of our
two test executables runs **3 cmocka unit tests** inside it. But CTest reports
only **2 tests** — because CTest sees two *executables*, not six *functions*. The
three assertions inside `test_math.exe` are invisible to CTest; it only sees
"`test_math.exe` exited with code 0, so `math_tests` passed." Hold onto that
2-vs-3 distinction — we'll see it play out concretely when we run things.

## Step by step

### 1. Write `test_math.c` — real assertions

Here is the complete math test. Read the top four includes carefully; they are
not optional decoration.

```c
/* These four includes are required by cmocka, in this order, before cmocka.h. */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "math_utils.h"

/* Each test takes (void **state) and uses cmocka assert_* macros. */
static void test_add_positive(void **state)
{
    (void)state;                    /* unused */
    assert_int_equal(add(2, 3), 5);
}

static void test_add_negative(void **state)
{
    (void)state;
    assert_int_equal(add(-2, -3), -5);
}

static void test_add_zero(void **state)
{
    (void)state;
    assert_int_equal(add(0, 0), 0);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_add_positive),
        cmocka_unit_test(test_add_negative),
        cmocka_unit_test(test_add_zero),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
```

The anatomy of a cmocka test:

- **The four includes, in that exact order, before `<cmocka.h>`.** cmocka's header
  uses types and macros defined by `stdarg.h` (`va_list`), `stddef.h`
  (`size_t`), and `setjmp.h` (`jmp_buf` — that's how cmocka jumps out of a test
  when an assertion fails). If you reorder them or drop one, you get cryptic
  compile errors *inside `cmocka.h`*. Treat the block as a fixed incantation.
- **Each test is a `static void` function taking `void **state`.** The `state`
  parameter is cmocka's per-test scratch pointer (for setup/teardown fixtures);
  we don't use it, so every test starts with `(void)state;` to silence the
  "unused parameter" warning.
- **`assert_int_equal(actual, expected)`** is the assertion. If the two ints
  differ, cmocka records a failure, prints where it happened, and longjmps out of
  the function. Pass means silence.
- **`main` builds an array of `CMUnitTest`**, wrapping each function with
  `cmocka_unit_test(...)`, and hands it to `cmocka_run_group_tests(tests, NULL,
  NULL)`. The two `NULL`s are group-level setup and teardown callbacks (none
  here). The return value of `cmocka_run_group_tests` becomes the program's exit
  code — which is exactly what CTest will read.

That's **3 unit tests** in **1 executable**.

### 2. Write `test_menus.c` — smoke tests

The `menus` functions (`print_main_menu`, `print_settings_menu`) return `void`
and only print to stdout. There's no return value to check, so a real assertion
is impossible. Instead we write **smoke tests**: call the function and treat "it
didn't crash" as a pass.

```c
/* Required by cmocka, in this order, before cmocka.h. */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdio.h>
#include "main_menu.h"
#include "settings_menu.h"

/*
 * These are "smoke tests": the menu functions return void and only print,
 * so there is no value to assert. We just call them and let the test pass
 * if they run without crashing. (Real assertions come once a function
 * actually returns data we can check.)
 */
static void test_main_menu_runs(void **state)
{
    (void)state;
    print_main_menu();
}

static void test_settings_menu_runs(void **state)
{
    (void)state;
    print_settings_menu();
}

/*
 * Prints every menu one after another so you can visually confirm the whole
 * output looks right. Run the exe directly to see it (ctest hides the output
 * of passing tests -- use `ctest -V` if you want it through ctest).
 */
static void test_print_all_menus(void **state)
{
    (void)state;
    printf("\n----- ALL MENUS -----\n");
    print_main_menu();
    printf("\n");
    print_settings_menu();
    printf("---------------------\n");
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_main_menu_runs),
        cmocka_unit_test(test_settings_menu_runs),
        cmocka_unit_test(test_print_all_menus),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
```

Same skeleton as the math test — the four includes, the `void **state`
functions, the `main` that runs a group. The difference is that no function calls
an `assert_*` macro; the tests pass simply by returning. A smoke test is a
legitimate, useful thing: it catches a function that segfaults, hits an infinite
loop, or fails to link. It just can't check *correctness of output* — for that
you need a function that returns data. (We'll get there in later chapters when we
start mocking.)

That's another **3 unit tests** in a **2nd executable**.

### 3. The reusable 5-step pattern

Every test target in `tests/CMakeLists.txt` follows the same recipe. Learn it
once and you can add a test for any module:

1. **`add_executable`** — a test is just a program with its own `main`.
2. **Add the cmocka include dir** so `#include <cmocka.h>` resolves.
3. **Link the code under test + `libcmocka`** — either compile the source
   straight in, or link the library that already contains it.
4. **POST_BUILD copy `cmocka.dll`** next to the `.exe` so it can launch.
5. **`add_test`** — register the exe with CTest.

## The CMake, explained

### The top `CMakeLists.txt`

Three additions turn testing on. First, near the top, right after `project(...)`:

```cmake
# Write build/compile_commands.json listing the exact compile flags for every
# source file. VS Code's C/C++ extension reads it so IntelliSense knows the
# include paths (stdint.h, cmocka.h, ...). Ninja/Makefile generators only.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

Then, at the very end of the file:

```cmake
# Turn on CTest support, then pull in the tests folder.
enable_testing()
add_subdirectory(tests)
```

- **`enable_testing()`** switches on CTest for the project. It must appear (in the
  top-level `CMakeLists.txt`) *before* any `add_test()` call runs, which is why
  it comes before `add_subdirectory(tests)`. Without it, your `add_test` calls
  are silently ignored and `ctest` finds nothing to run.
- **`add_subdirectory(tests)`** tells CMake to descend into `tests/` and process
  its `CMakeLists.txt` — exactly like `add_subdirectory(menus)` did for the
  library back in Chapter 4.
- **`CMAKE_EXPORT_COMPILE_COMMANDS`** we'll cover in its own section below.

### `tests/CMakeLists.txt`, line by line

Here's the whole file. It's the first CMake file in this book that does something
genuinely new, so we'll walk every block.

```cmake
# --- Unit tests (cmocka) ---

# Where cmocka is installed on this machine (a mingw/gcc build).
set(CMOCKA_DIR "C:/MinGW/cmocka")

# --- math module test ---
# For now we compile math_utils.c straight into the test (simplest). Once
# math_utils becomes its own library (a later episode) we'll link that instead.
add_executable(test_math
    test_math.c
    ${CMAKE_SOURCE_DIR}/Src/math_utils.c
)

target_include_directories(test_math PRIVATE
    ${CMAKE_SOURCE_DIR}/Inc      # so it finds math_utils.h
    ${CMOCKA_DIR}/include        # so it finds cmocka.h
)

# Link the cmocka library.
target_link_libraries(test_math PRIVATE ${CMOCKA_DIR}/lib/libcmocka.dll.a)

# cmocka is a DLL: copy it next to the test .exe so the test runs standalone.
add_custom_command(TARGET test_math POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMOCKA_DIR}/bin/cmocka.dll $<TARGET_FILE_DIR:test_math>
)

# Register the exe with CTest so "ctest" runs it.
add_test(NAME math_tests COMMAND test_math)


# --- menus module test ---
# menus is already a library target, so we just link it (no source files here).
add_executable(test_menus test_menus.c)

target_include_directories(test_menus PRIVATE ${CMOCKA_DIR}/include)

# Link the menus library (brings its Inc/ headers along, since they're PUBLIC)
# and cmocka.
target_link_libraries(test_menus PRIVATE menus ${CMOCKA_DIR}/lib/libcmocka.dll.a)

add_custom_command(TARGET test_menus POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMOCKA_DIR}/bin/cmocka.dll $<TARGET_FILE_DIR:test_menus>
)

add_test(NAME menus_tests COMMAND test_menus)
```

**`set(CMOCKA_DIR "C:/MinGW/cmocka")`** — cmocka isn't part of CMake or the
compiler; it's a third-party library installed somewhere on this machine. We
capture its root in a variable once so the paths below read cleanly. On your
machine this path may differ — that's the one line you'd edit. (Forward slashes
work fine in CMake even on Windows; prefer them over backslashes.)

**`add_executable(test_math test_math.c ${CMAKE_SOURCE_DIR}/Src/math_utils.c)`** —
this is step 1 and part of step 3 at once. A test *is* a program, so we build one.
Notice we compile the module under test, `math_utils.c`, **straight into the test
executable**. `${CMAKE_SOURCE_DIR}` is the top of the project (where the root
`CMakeLists.txt` lives), so this reaches back up to `Src/math_utils.c` from inside
`tests/`. Compiling the source in directly is the simplest thing that works. The
comment notes this changes later — in **Chapter 10**, once `math_utils` becomes
its own library target, we'll *link* that library instead of re-listing its
source here. (The committed comment says "a later episode"; that later episode is
Chapter 10.)

**`target_include_directories(test_math PRIVATE ${CMAKE_SOURCE_DIR}/Inc
${CMOCKA_DIR}/include)`** — step 2. The test needs two search paths: our own
`Inc/` (so `#include "math_utils.h"` resolves) and cmocka's `include/` (so
`#include <cmocka.h>` resolves). `PRIVATE` because nothing links *against* a test
executable, so these paths don't need to propagate anywhere.

**`target_link_libraries(test_math PRIVATE ${CMOCKA_DIR}/lib/libcmocka.dll.a)`** —
the rest of step 3. `libcmocka.dll.a` is the *import library* for the cmocka DLL —
a small stub the linker uses to record "this exe needs symbols from cmocka.dll at
runtime." Linking it is what lets `cmocka_run_group_tests` and the `assert_*`
macros resolve.

**The `add_custom_command(... POST_BUILD ...)`** — step 4, and the one that trips
people up. Because cmocka is a **DLL** (a shared library), linking the import
library is not enough: when the `.exe` runs, Windows must find `cmocka.dll` at
runtime, and it looks first in the exe's own folder. So immediately after the test
is built (`POST_BUILD`), we run a copy. We use CMake's own cross-platform copy,
`${CMAKE_COMMAND} -E copy_if_different`, rather than a shell `copy`, so it works
on any OS. `copy_if_different` skips the copy when the DLL is already there and
unchanged (no needless work on rebuilds). The destination,
`$<TARGET_FILE_DIR:test_math>`, is a *generator expression* — the same kind you
met in Chapter 5 — that expands at build time to "the directory the `test_math`
executable lands in" (here, `build/tests/`). We don't hard-code the path because
it depends on the generator and configuration.

**`add_test(NAME math_tests COMMAND test_math)`** — step 5. This is the whole
bridge to CTest: give the registered test a name (`math_tests`) and the command
to run (the `test_math` target). From now on, `ctest` will launch this exe and
grade it by its exit code. Note the CTest name (`math_tests`) and the target name
(`test_math`) are deliberately different so we can talk about them separately.

**The `test_menus` block** is the same five steps with one instructive change: we
**don't** recompile any source. `menus` is already a library target (from
`add_subdirectory(menus)` in Chapter 4), so we just
`target_link_libraries(test_menus PRIVATE menus ...)`. Because `menus` marked its
`Inc/` directory `PUBLIC`, linking the library automatically drags its header
search path along — which is why `test_menus` can `#include "main_menu.h"`
without us adding `menus/Inc` to `target_include_directories`. This is the payoff
of building a proper library target back in Chapter 4: consumers (including tests)
get the headers for free. The only include dir we add by hand is cmocka's.

### `CMAKE_EXPORT_COMPILE_COMMANDS` and `.vscode/settings.json`

Here's a problem you'll hit the moment you open `test_math.c` in VS Code: a red
squiggle under `#include <cmocka.h>`, and IntelliSense complaining it can't find
the file. The code *compiles fine* — the compiler knows the include paths because
CMake passed them. But the **editor** is a separate program that has no idea what
flags CMake used. It's guessing, and it guesses wrong for a library installed off
in `C:/MinGW/cmocka/include`.

The fix is two pieces:

**1. `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`** tells CMake to write a file called
`compile_commands.json` into `build/`. This is a machine-readable list of *every*
source file in the project and the *exact* compiler command line used to build it
— every `-I` include path, every flag. It's the ground truth of how each file is
actually compiled. (This feature only works with the Ninja and Makefile
generators — which is what we use — not the Visual Studio generator.)

**2. `.vscode/settings.json`** points the C/C++ extension at that file:

```json
{
  "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json"
}
```

Now IntelliSense reads the same include paths the compiler used, the squiggle
disappears, and go-to-definition works into `cmocka.h`. The editor and the build
finally agree, because they're reading from one source of truth. Note this file
only exists after you've configured the project at least once (that's when CMake
writes it), so if the squiggles persist, run the `cmake -S . -B build ...` step
and reload VS Code.

## Build and run

Configure and build exactly as before — the new test targets get built alongside
`hello`:

```bash
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
```

There are **two ways** to run the tests, and seeing both makes the
cmocka-vs-CTest split concrete.

**Way 1 — run a test exe directly.** This shows you cmocka's own output, the exam
paper itself:

```text
$ ./build/tests/test_math.exe
[==========] Running 3 test(s).
[ RUN      ] test_add_positive
[       OK ] test_add_positive
[ RUN      ] test_add_negative
[       OK ] test_add_negative
[ RUN      ] test_add_zero
[       OK ] test_add_zero
[==========] 3 test(s) run.
[  PASSED  ] 3 test(s).
```

That `[ RUN ]` / `[ OK ]` / `[ PASSED ]` banner is pure cmocka — CTest is not
involved at all here. Notice it says **3 test(s)**: the three functions we wrote.

**Way 2 — run everything through CTest.** CTest must be run from *inside* the
`build/` directory (that's where it finds the test registry CMake generated):

```text
$ cd build
$ ctest
Test project C:/Users/tal.g/.../cmake-course/build
    Start 1: math_tests
1/2 Test #1: math_tests .......................   Passed    0.02 sec
    Start 2: menus_tests
2/2 Test #2: menus_tests ......................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   0.04 sec
```

Here's the 2-vs-3 payoff. CTest says **out of 2** — it counts the two things we
registered with `add_test` (`math_tests`, `menus_tests`), one per executable. The
*three* cmocka tests inside each exe are invisible to it; CTest only ever saw two
programs and two exit codes. cmocka counts functions; CTest counts programs.

And notice CTest printed **none** of the cmocka `[ RUN ]/[ OK ]` output above.
**CTest hides the output of passing tests** — a clean run is meant to be quiet. If
you want to see it, either run the exe directly (Way 1) or ask CTest for it:

```text
ctest -V                     # verbose: show all output, even for passing tests
ctest --output-on-failure    # show output only for tests that FAIL
```

`test_menus`'s `test_print_all_menus` prints all the menus — you'll only see that
through `ctest -V` or by running `./build/tests/test_menus.exe` directly.

## What's autogenerated

Everything below lands in `build/` and is disposable — never commit it. New this
chapter:

- **`build/compile_commands.json`** — written because we set
  `CMAKE_EXPORT_COMPILE_COMMANDS ON`. The per-file compiler command database that
  feeds VS Code IntelliSense. Regenerated on every configure.
- **`build/tests/test_math.exe` and `build/tests/test_menus.exe`** — the two test
  programs, built like any other executable.
- **`build/tests/cmocka.dll`** — the copy our `POST_BUILD` custom command drops
  next to each test exe so it can launch. You didn't put it there; CMake did,
  every build.

The `.vscode/settings.json` and the `tests/` sources, by contrast, are files
*you* wrote and *do* get committed — they're inputs, not outputs.

## Gotchas

- **The four cmocka includes must come before `<cmocka.h>`, in this order:**
  `<stdarg.h>`, `<stddef.h>`, `<setjmp.h>`, then `<cmocka.h>`. `cmocka.h` depends
  on types those three headers define. Reorder them, drop one, or put your own
  headers first, and you get baffling compile errors pointing *inside* `cmocka.h`,
  not at your code.
- **No `cmocka.dll` copy = the test won't even launch.** Skip the `POST_BUILD`
  custom command and the build succeeds, but running the exe fails immediately
  with a "cmocka.dll not found" error (or a silent non-launch) before a single
  test runs. The DLL must sit next to the `.exe`.
- **`ctest` must be run from the `build/` directory.** Run it from the project
  root and it prints something like "No tests were found" — the test registry
  lives under `build/`, and that's where CTest looks.
- **A void-returning print function can only be smoke-tested.** `test_menus` has
  no real assertions because `print_main_menu()` returns nothing to check. That's
  a limitation of the code under test, not a mistake — real assertions require a
  function that hands back a value. Don't fake an assertion just to have one.
- **`enable_testing()` must come before `add_test`.** It's in the top-level
  `CMakeLists.txt` before `add_subdirectory(tests)` for exactly this reason. Put
  the `add_test` calls first and CTest quietly registers nothing.
- **`CMAKE_EXPORT_COMPILE_COMMANDS` needs a re-configure to take effect**, and
  only the Ninja/Makefile generators write the file. If VS Code still squiggles,
  make sure `build/compile_commands.json` actually exists, then reload the window.

## Recap

- **cmocka is the framework**: it lives inside each test `.c`, provides the four
  required includes, `assert_int_equal`, `cmocka_unit_test`, and
  `cmocka_run_group_tests`, and reports pass/fail through the program's **exit
  code**.
- **CTest is the runner** that ships with CMake: it knows nothing about cmocka —
  it just launches each exe registered with `add_test` and checks the exit code
  (`0` = pass). *cmocka is the exam; CTest is the teacher.*
- That's why our two exes hold **3 cmocka tests each** but `ctest` reports **2
  tests** — CTest counts executables, cmocka counts functions.
- Enabling tests is three lines in the top `CMakeLists.txt`:
  `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`, `enable_testing()` (before any
  `add_test`), and `add_subdirectory(tests)`.
- Every test target follows the same **5-step pattern**: `add_executable` → add
  the cmocka include dir → link the code-under-test + `libcmocka` → `POST_BUILD`
  copy `cmocka.dll` → `add_test`.
- `test_math` compiles `math_utils.c` **straight in** (simplest for now; becomes a
  library link in Chapter 10); `test_menus` **links the `menus` library** and
  inherits its `PUBLIC` headers for free.
- `CMAKE_EXPORT_COMPILE_COMMANDS` writes `build/compile_commands.json`, and
  `.vscode/settings.json` points the C/C++ extension at it — that's what kills the
  `#include <cmocka.h>` squiggle.
- Run a test exe directly to see cmocka's `[ RUN ]/[ OK ]/[ PASSED ]` output; run
  `ctest` from `build/` for the tally. CTest hides passing output — use `ctest -V`
  or `ctest --output-on-failure` to see it.

## Get this episode

```bash
git checkout ep06-cmocka-ctest
export PATH="/c/MinGW/w64devkit/bin:$PATH"
cmake -S . -B build -G Ninja -D CMAKE_C_COMPILER=gcc
cmake --build build
cd build && ctest --output-on-failure
```

---
[← Chapter 5: Generators and commands](05-generators-commands.md) · [Chapter 7: Mocking a hardware leaf →](07-mocking-adc.md)
