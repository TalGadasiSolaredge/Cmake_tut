#include "adc.h"
#include "app_config.h"        /* generated: provides ADC_VREF_MV */

#define ADC_MAX_COUNT 4095.0f  /* 12-bit full scale */

/*
 * The function under test. It contains NO hardware access itself -- it only
 * calls adc_read_raw() (the leaf) and does the conversion math. That split is
 * exactly what lets a test mock adc_read_raw() and check this math in isolation.
 *
 * Vref now comes from the build config (ADC_VREF_MV, in millivolts) so it can
 * be changed with -D ADC_VREF_MV=3300 instead of editing this file.
 */
float adc_read_voltage(void)
{
    uint16_t raw = adc_read_raw();
    return ((float)raw / ADC_MAX_COUNT) * (ADC_VREF_MV / 1000.0f);
}
