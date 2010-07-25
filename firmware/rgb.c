#include "rgb.h"

#include <avr/io.h>

#include "bitfield.h"

#include <stdio.h>

// #define RGB_COM		GET_BIT(PORTD).bit5
#define RGB_COM		GET_BIT(PORTD).bit7
#define LED_RED		GET_BIT(PORTA).bit0
#define LED_GREEN	GET_BIT(PORTA).bit1
#define LED_BLUE	GET_BIT(PORTA).bit2

#define LED_ENABLED	0
#define LED_DISABLED	1


#define TIMER2_PWM_INIT		_BV(WGM21) | _BV(WGM20) | _BV(COM21) | _BV(CS21) | _BV(CS20)	/* 8 Mhz / 32 = 250 Khz */

#define OCR			OCR2
#define DDROC			DDRD
#define OC1A			PD7
#define TIMER2_TOP		255

void 
rgb_init(void)
{
	printf("rgb_init()\n");

	/* Enable LEDs port as output. */
	DDRA |= (_BV(PA0) | _BV(PA1) | _BV(PA2));
	DDRD |= _BV(PD7);
	rgb_set(RGB_OFF);
}

#if 0
void
rgb_pwm_start(void)
{
	/* Enable OC2 as output. */
	DDROC |= _BV(OC1A);

	/* Timer 1 is 16-bit PWM. */
	TCCR2 = TIMER1_PWM_INIT;

	/* Set PWM value to 0. */
	OCR = 0;

	/* Enable timer 2 overflow interrupt. */
// 	TIFR &= ~(_BV (TOV2));
	TIMSK |= _BV(TOIE2);
}

void
rgb_pwm_stop(void)
{
	/* Timer 2 is 8-bit PWM. */
	TCCR2 = 0x00;

	/* Disable timer 2 overflow interrupt. */
	TIMSK &= ~(_BV(TOIE2));

	/* Enable PD7 as output. */
	DDRD |= _BV(PD7);
}
#endif /* 0 */

void
rgb_set(const rgb_mode_t mode)
{
	switch(mode) {
		case RGB_OFF:
				// rgb_pwm_stop();
				RGB_COM = 1;
				LED_RED = LED_DISABLED;
				LED_GREEN = LED_DISABLED;
				LED_BLUE = LED_DISABLED;
				break;
		case RGB_BLUE:
				// rgb_pwm_stop();
				RGB_COM = 0;
				LED_RED = LED_DISABLED;
				LED_GREEN = LED_DISABLED;
				LED_BLUE = LED_ENABLED;
				break;
		case RGB_GREEN:
				// rgb_pwm_stop();
				RGB_COM = 0;
				LED_RED = LED_DISABLED;
				LED_GREEN = LED_ENABLED;
				LED_BLUE = LED_DISABLED;
				break;
		case RGB_YELLOW:
				// rgb_pwm_stop();
				RGB_COM = 0;
				LED_RED = LED_ENABLED;
				LED_GREEN = LED_ENABLED;
				LED_BLUE = LED_DISABLED;
				break;
		case RGB_RED:
				// rgb_pwm_stop();
				RGB_COM = 0;
				LED_RED = LED_ENABLED;
				LED_GREEN = LED_DISABLED;
				LED_BLUE = LED_DISABLED;
				break;
	};
}
