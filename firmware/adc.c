#include "adc.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdbool.h>

/**
 * Software flag used to indicate when the A/D conversion is complete.
 */
static volatile bool _adc_convertion_done;

/** Interrupt handler for ADC complete interrupt. */
ISR(ADC_vect)
{
  /* set the a2d conversion flag to indicate "complete" */
  _adc_convertion_done = true;
}


/* initialize A/D converter */
void
adc_init(void)
{
  DDRC &= ~(_BV(PC0));

  ADMUX = _BV(REFS0) | _BV(ADLAR) | _BV(MUX2) | _BV(MUX1) | _BV(MUX0);	/* set reference voltage to VCC,
										   set to left-adjusted result
										   select channel ADC7 */
  ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADIE);	/* enable ADC (turn on ADC power),
								   set prescaler division factor to 64,
								   enable adc interrupt */
  _adc_convertion_done = false;	/* clear conversion complete flag */
}

uint8_t
adc_convert(void)
{
  ADCSRA |= _BV(ADSC);		/* start conversion */
  while (bit_is_set(ADCSRA, ADSC));	/* wait until conversion complete */
  return (ADCH);		/* read only 8 most signifiant bits */
}
