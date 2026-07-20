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
