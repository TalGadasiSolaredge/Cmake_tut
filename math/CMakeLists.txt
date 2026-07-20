# --- The "math_utils" module ---
# math_utils is now its own library, so ANY target can link it: the app, the
# menus library, and the unit test all share this one definition of add().
add_library(math_utils
    Src/math_utils.c
)

target_include_directories(math_utils PUBLIC Inc)
