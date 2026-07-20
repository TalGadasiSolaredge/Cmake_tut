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
