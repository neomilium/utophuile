#include "buttons.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include "beep.h"
#include "scheduler.h"

typedef enum {
		BUTTON_RELEASED,
		BUTTON_PRESSED
} button_state_t;
static volatile button_state_t _button0_state = BUTTON_RELEASED;
static volatile uint8_t _button0_pressed_counter = 0;
static volatile button_state_t _button1_state = BUTTON_RELEASED;
static volatile uint8_t _button1_pressed_counter = 0;

static volatile button_action_t _buttons_requested_action = BUTTON_ACTION_NONE;

void buttons_process(void);

ISR(INT0_vect)
{
	if ( bit_is_set( PIND, PD2 ) ) {
		_button0_state = BUTTON_RELEASED;
		if ( _button0_pressed_counter < 2 ) {
			_buttons_requested_action = BUTTON_ACTION_OK;
		} else {
			
		}
	} else {
		_button0_state = BUTTON_PRESSED;
		_button0_pressed_counter = 0;
	}
}

ISR(INT1_vect)
{
	if ( bit_is_set( PIND, PD3 ) ) {
		_button1_state = BUTTON_RELEASED;
		if ( _button1_pressed_counter < 2 ) {
			_buttons_requested_action = BUTTON_ACTION_LIGHT;
		} else {
			
		}
	} else {
		_button1_state = BUTTON_PRESSED;
		_button1_pressed_counter = 0;
	}
}

void
buttons_init(void)
{
	/* Dashboard button */
	DDRD &= ~(_BV(PD2));		// PD2 as input
	PORTD |= _BV(PD2);		// Enable internal pull up

	MCUCR |= _BV( ISC00 );		// Enable INT0 on both failing and rising edge
	GICR |= _BV( INT0 );

	/* Gauge button */
	DDRD &= ~(_BV(PD3));            // PD3 as input
	PORTD |= _BV(PD3);		// Enable internal pull up

	MCUCR |= _BV( ISC10 );		// Enable INT1 on both failing and rising edge
	GICR |= _BV( INT1 );

	scheduler_add_hook_fct ( buttons_process );
}

button_action_t
buttons_get_requested_action(void)
{
	const button_action_t buttons_requested_action = _buttons_requested_action;
	_buttons_requested_action = BUTTON_ACTION_NONE;
	return buttons_requested_action;
}

void
buttons_process(void)
{
	if ( _button0_state == BUTTON_PRESSED ) {
		if(++_button0_pressed_counter >= 2) {
			_buttons_requested_action = BUTTON_ACTION_POWER;
			_button0_state = BUTTON_RELEASED;
		}
	}
}
