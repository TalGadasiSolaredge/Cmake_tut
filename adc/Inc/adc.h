#ifndef ADC_H
#define ADC_H

#include <stdint.h>

/*
 * Hardware leaf: returns one raw 12-bit ADC sample (0..4095).
 * Real implementation lives in adc_hal.c. In unit tests this symbol is
 * REPLACED by a mock so we can feed the logic any value we like.
 */
uint16_t adc_read_raw(void);

/*
 * Reads the ADC and converts the raw count to volts.
 * 12-bit full scale = 4095 counts, reference voltage Vref = 3.0 V.
 * Example: raw 4095 -> 3.0 V, raw 0 -> 0.0 V, raw 2048 -> ~1.5 V.
 */
float adc_read_voltage(void);

#endif /* ADC_H */
