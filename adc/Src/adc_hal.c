#include "adc.h"

/*
 * The REAL hardware read. On a real MCU this would start a conversion and read
 * the ADC data register, e.g.:
 *
 *     ADC1->CR |= ADC_CR_ADSTART;
 *     while (!(ADC1->ISR & ADC_ISR_EOC)) { }
 *     return (uint16_t)ADC1->DR;
 *
 * There is no hardware here, so we return a fixed placeholder. This file is
 * linked into the app but NOT into the unit test -- the test supplies its own
 * adc_read_raw() mock instead.
 */
uint16_t adc_read_raw(void)
{
    return 2048u;   /* pretend the pin sits at mid-scale (~1.5 V) */
}
