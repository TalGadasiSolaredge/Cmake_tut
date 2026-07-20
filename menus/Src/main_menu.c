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
