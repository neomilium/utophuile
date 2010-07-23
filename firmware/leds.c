#include "leds.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include "bitfield.h"

#include "scheduler.h"

#include <stdio.h>

#define LEDS_COM	GET_BIT(PORTD).bit7
#define LED2		GET_BIT(PORTC).bit2
#define LED1		GET_BIT(PORTC).bit3
#define LED0		GET_BIT(PORTC).bit4

#define LED_ENABLED	0
#define LED_DISABLED	1

#define BLINK_MASK	0x80

enum { UP, DOWN };

#define TIMER2_PWM_INIT		_BV(WGM21) | _BV(WGM20) | _BV(COM21) | _BV(CS21) | _BV(CS20)	/* 8 Mhz / 32 = 250 Khz */

#define OCR			OCR2
#define DDROC			DDRD
#define OC2			PD7
#define TIMER2_TOP		255

static leds_mode_t _leds_mode = LED_ALL_OFF;

ISR (TIMER2_OVF_vect)
{
	static uint16_t pwm;
	static uint8_t direction;

	switch (direction)
	{
	case UP:
		if (++pwm == TIMER2_TOP)
		direction = DOWN;
		break;
	
	case DOWN:
		if (--pwm == 0)
		direction = UP;
		break;
	}
	if ( pwm < 50 ) {
		OCR = 0;
	} else {
		OCR = pwm;
	}
}

void leds_process(void);

void 
leds_init(void)
{
	printf("leds_init()\n");

	/* Enable LEDs port as output. */
	DDRC |= (_BV(PC2) | _BV(PC3) | _BV(PC4));

	/* Set output direction for PD7 */
        DDRD |= _BV(PD7);

	scheduler_add_hook_fct( leds_process );
}

void
leds_pwm_start(void)
{
	/* Enable OC2 as output. */
	DDROC |= _BV(OC2);

	/* Timer 2 is 8-bit PWM. */
	TCCR2 = TIMER2_PWM_INIT;

	/* Set PWM value to 0. */
	OCR = 0;

	/* Enable timer 2 overflow interrupt. */
// 	TIFR &= ~(_BV (TOV2));
	TIMSK |= _BV(TOIE2);
}

void
leds_pwm_stop(void)
{
	/* Timer 2 is 8-bit PWM. */
	TCCR2 = 0x00;

	/* Disable timer 2 overflow interrupt. */
	TIMSK &= ~(_BV(TOIE2));

	/* Enable PD7 as output. */
	DDRD |= _BV(PD7);
}


void
leds_set(const leds_mode_t mode)
{
	switch(mode) {
		case LED_ALL_OFF:
// 				leds_pwm_stop();
				LEDS_COM = 1;
				LED0 = LED_DISABLED;
				LED1 = LED_DISABLED;
				LED2 = LED_DISABLED;
				break;
		case LED_ALL_BLINK:
// 				leds_pwm_start();
				LED0 = LED_ENABLED;
				LED1 = LED_ENABLED;
				LED2 = LED_ENABLED;
				break;
		case LED_GREEN_ON:
// 				leds_pwm_stop();
				LEDS_COM = 1;
				LED0 = LED_ENABLED;
				LED1 = LED_DISABLED;
				LED2 = LED_DISABLED;
				break;
		case LED_GREEN_BLINK:
// 				leds_pwm_start();
				LED0 = LED_ENABLED;
				LED1 = LED_DISABLED;
				LED2 = LED_DISABLED;
				break;
		case LED_ORANGE_ON:
// 				leds_pwm_stop();
				LEDS_COM = 1;
				LED0 = LED_DISABLED;
				LED1 = LED_ENABLED;
				LED2 = LED_DISABLED;
				break;
		case LED_ORANGE_BLINK:
// 				leds_pwm_start();
				LED0 = LED_DISABLED;
				LED1 = LED_ENABLED;
				LED2 = LED_DISABLED;
				break;
		case LED_RED_ON:
// 				leds_pwm_stop();
				LEDS_COM = 1;
				LED0 = LED_DISABLED;
				LED1 = LED_DISABLED;
				LED2 = LED_ENABLED;
				break;
		case LED_RED_BLINK:
// 				leds_pwm_start();
				LED0 = LED_DISABLED;
				LED1 = LED_DISABLED;
				LED2 = LED_ENABLED;
				break;
	};
	_leds_mode = mode;
}

void
leds_process( void )
{
	if ( _leds_mode & BLINK_MASK ) {
		if ( LEDS_COM ) {
			LEDS_COM = 0;
		} else {
			LEDS_COM = 1;
		}
	}
}

