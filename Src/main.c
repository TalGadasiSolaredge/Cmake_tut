#include <stdio.h>
#include "math_utils.h"
#include "main_menu.h"
#include "settings_menu.h"
#include "adc.h"

#ifdef ENABLE_DEBUG_MENU
#include "debug_menu.h"
#endif

int main(void)
{
    printf("Hello, World!\n");
    printf("2 + 3 = %d\n", add(2, 3));
    printf("ADC voltage = %.3f V\n\n", adc_read_voltage());

    print_main_menu();
    printf("\n");
    print_settings_menu();

#ifdef ENABLE_DEBUG_MENU
    printf("\n");
    print_debug_menu();
#endif
    return 0;
}
