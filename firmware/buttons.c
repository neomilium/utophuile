#include "buttons.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdio.h>

#include "beep.h"
#include "scheduler.h"

#define BUTTONS_PORT 	PORTD
#define BUTTONS_PIN	PIND
#define BUTTONS_DDR 	DDRD
#define BUTTON0 	PD2

typedef enum {
  BUTTON_RELEASED,
  BUTTON_PRESSED
} button_state_t;
static volatile button_state_t _button0_state = BUTTON_RELEASED;
static volatile uint8_t _button0_pressed_counter = 0;

static volatile button_action_t _buttons_requested_action = BUTTON_ACTION_NONE;

void buttons_process(void);

ISR(INT0_vect)
{
  if (bit_is_set(BUTTONS_PIN, BUTTON0)) {
    _button0_state = BUTTON_RELEASED;
    if (_button0_pressed_counter < 2) {
      _buttons_requested_action = BUTTON_ACTION_OK;
    } else {

    }
  } else {
    _button0_state = BUTTON_PRESSED;
    _button0_pressed_counter = 0;
  }
}

void
buttons_init(void)
{
  /* Dashboard button */
  BUTTONS_DDR &= ~(_BV(BUTTON0));		// BUTTON0 as input
  BUTTONS_PORT |= _BV(BUTTON0);		// Enable internal pull up

  EICRA |= _BV(ISC00);		// Enable INT0 on both failing and rising edge
  EIMSK |= _BV(INT0);

  scheduler_add_hook_fct(buttons_process);
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
  if (_button0_state == BUTTON_PRESSED) {
    if (++_button0_pressed_counter >= 2) {
      _buttons_requested_action = BUTTON_ACTION_POWER;
      _button0_state = BUTTON_RELEASED;
    }
  }
}
