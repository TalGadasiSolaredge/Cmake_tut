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

# Only when the option is ON:
if(ENABLE_DEBUG_MENU)
    # 1) compile the extra source into this library
    target_sources(menus PRIVATE Src/debug_menu.c)

    # 2) define the preprocessor macro ENABLE_DEBUG_MENU (this is what turns
    #    -D<name> on for the compiler). PUBLIC means anything linking "menus"
    #    -- like the app -- ALSO gets the macro, so main.c's #ifdef sees it.
    target_compile_definitions(menus PUBLIC ENABLE_DEBUG_MENU)
endif()
