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
