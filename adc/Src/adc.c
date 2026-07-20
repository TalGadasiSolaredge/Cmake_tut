#include "adc.h"

#define ADC_MAX_COUNT 4095.0f   /* 12-bit full scale */
#define ADC_VREF      3.0f      /* reference voltage in volts */

/*
 * The function under test. It contains NO hardware access itself -- it only
 * calls adc_read_raw() (the leaf) and does the conversion math. That split is
 * exactly what lets a test mock adc_read_raw() and check this math in isolation.
 */
float adc_read_voltage(void)
{
    uint16_t raw = adc_read_raw();
    return ((float)raw / ADC_MAX_COUNT) * ADC_VREF;
}
