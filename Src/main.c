#include <stdio.h>
#include "app_config.h"        /* generated: all 5 build flags */
#include "math_utils.h"
#include "main_menu.h"
#include "settings_menu.h"
#include "adc.h"

#ifdef ENABLE_DEBUG_MENU
#include "debug_menu.h"
#endif

int main(void)
{
#ifdef ENABLE_STARTUP_BANNER
    printf("==============================\n");
    printf("   Hello App   v%s\n", APP_VERSION);   /* value flag (string) */
    printf("==============================\n");
#endif

    printf("Hello, World!\n");
    printf("2 + 3 = %d\n", add(2, 3));
    printf("ADC voltage = %.3f V\n", adc_read_voltage());

#if VERBOSE_LOGGING                                  /* 0/1 flag -> use #if */
    printf("[verbose] Vref = %d mV\n", ADC_VREF_MV); /* value flag (int) */
#endif
    printf("\n");

    print_main_menu();
    printf("\n");
    print_settings_menu();

#ifdef ENABLE_DEBUG_MENU                             /* on/off flag -> use #ifdef */
    printf("\n");
    print_debug_menu();
#endif
    return 0;
}
