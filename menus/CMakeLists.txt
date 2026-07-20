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
