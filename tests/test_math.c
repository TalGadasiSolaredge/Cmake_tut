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
