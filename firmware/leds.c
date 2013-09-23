#include "leds.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include "bitfield.h"

#include "scheduler.h"

#include <stdio.h>

// Common LEDs (Low = active, High = inactive)
#define LEDS_COM	GET_BIT(PORTC).bit0 	// Arduino Analog Input 0
// Green LED
#define LED0		GET_BIT(PORTC).bit1 	// Arduino Analog Input 1
// Orange LED
#define LED1		GET_BIT(PORTC).bit2 	// Arduino Analog Input 2
// Red LED
#define LED2		GET_BIT(PORTC).bit3 	// Arduino Analog Input 3

#define LED_ENABLED	0
#define LED_DISABLED	1

#define BLINK_MASK	0x80

enum { UP, DOWN };

static leds_mode_t _leds_mode = LED_ALL_OFF;

void leds_process(void);

void
leds_init(void)
{
  /* Enable LEDs port as output. */
  DDRC |= (_BV(PC0) | _BV(PC1) | _BV(PC2) | _BV(PC3));

  scheduler_add_hook_fct(leds_process);
}

void
leds_set(const leds_mode_t mode)
{
  switch (mode) {
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
leds_process(void)
{
  if (_leds_mode & BLINK_MASK) {
    if (LEDS_COM) {
      LEDS_COM = 0;
    } else {
      LEDS_COM = 1;
    }
  }
}

