#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#include "utophuile.h"

#include "adc.h"
#include "leds.h"
#include "rgb.h"
#include "beep.h"
#include "buttons.h"
#include "uart.h"
#include "twi.h"
#include "relay.h"

#include "scheduler.h"

#include "config.h"

// FILE uart_stdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_stdio = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

typedef enum {
  UTOPHUILE_MODE_OFF,
  UTOPHUILE_MODE_HEATING,
  UTOPHUILE_MODE_READY,
  UTOPHUILE_MODE_OIL,
  UTOPHUILE_MODE_EMERGENCY,
  UTOPHUILE_MODE_ERROR
} utophuile_mode_t;

void utophuile_process(void);
void utophuile_set_mode(utophuile_mode_t mode);
uint8_t utophuile_oil_temperature(void);


#ifdef SIMULATE_TEMP
static volatile uint8_t _fake_utophuile_oil_temperature = 75;
#endif

static volatile utophuile_mode_t _utophuile_mode = UTOPHUILE_MODE_OFF;
static volatile utophuile_mode_t _utophuile_previous_mode = UTOPHUILE_MODE_OFF;

typedef enum {
  UTOPHUILE_ALERTER_ENABLED,
  UTOPHUILE_ALERTER_DISABLED,
} alerter_mode_t;
static volatile alerter_mode_t _utophuile_alerter_mode = UTOPHUILE_ALERTER_ENABLED;

#define UTOPHUILE_TOLERENCE_OIL_TEMPERATURE 3
#define UTOPHUILE_MIN_OIL_TEMPERATURE  59 /* Stop when < MIN_OIL_TEMP, ready when > ( MIN_OIL_TEMP + TOLERENCE ) */
#define UTOPHUILE_MAX_OIL_TEMPERATURE  94 /* Stop when > MAX_OIL_TEMP, ready when < ( MAX_OIL_TEMP + TOLERENCE ) */

#define GAUGE_TEMP_STAGE1	UTOPHUILE_MIN_OIL_TEMPERATURE + 1
#define GAUGE_TEMP_STAGE2	90
#define GAUGE_TEMP_STAGE3	UTOPHUILE_MAX_OIL_TEMPERATURE + 1

static uint8_t _report_mode_enabled = 0;

int
main(void)
{
  cli();

  uart_init();
  stdout = stdin = &uart_stdio;
  stderr = &uart_stdio;

  printf_P(PSTR("\n"PACKAGE_STRING"\n"));

  scheduler_init();

  buttons_init();
  leds_init();
  rgb_init();
  beep_init();
  adc_init();
  twi_init();
  relay_init();

  scheduler_add_hook_fct(utophuile_process);

  sei();   /* Enable interrupts */

  utophuile_set_mode(UTOPHUILE_MODE_OFF);

  beep_play_partition_P(PSTR("A_A"));

  static char buf[20], s[20];
  uint8_t pcf_data;
  int d;
  unsigned int x;

  printf_P(PSTR("$ "));
  for (;;) {
    if (fgets(buf, sizeof(buf) - 1, stdin) == NULL) {
      clearerr(stdin);
      continue;
    }

    switch (tolower(buf[0])) {
      case '\n':
        break;
      default:
        printf_P(PSTR("unknown command: %c"), buf[0]);
        break;

      case 'g': /* Gauge */
        if (sscanf(buf, "%*s %s", s) > 0) {
          switch (s[0]) {
            case '0':
              rgb_set(RGB_OFF);
              break;
            case 'b':
              rgb_set(RGB_BLUE);
              break;
            case 'g':
              rgb_set(RGB_GREEN);
              break;
            case 'y':
              rgb_set(RGB_YELLOW);
              break;
            case 'r':
              rgb_set(RGB_RED);
              break;
          }
        }
        break;
      case 'l': /* Leds */
        if (sscanf(buf, "%*s %s", s) > 0) {
          switch (s[0]) {
            case 'g':
              leds_set(LED_GREEN_BLINK);
              break;
            case 'r':
              leds_set(LED_RED_BLINK);
              break;
            case 'o':
              leds_set(LED_ORANGE_BLINK);
              break;
          }
        }
        break;
      case 'p': /* PCF (Relays) */
        if (sscanf(buf, "%*s %x", &x) > 0) {
          pcf_data = x;
          if (-1 != twi_write_bytes(0x40, 1, &pcf_data)) {
            printf("write %#02x to pcf data", pcf_data);
          } else {
            printf_P(PSTR("unable to write to pcf"));
          }
        } else {
          if (-1 != twi_read_bytes(0x40, 1, &pcf_data)) {
            printf("pcf data = %#02x", pcf_data);
          } else {
            printf_P(PSTR("unable to read pcf data"));
          }
        }
        break;
      case 'r': /* Report mode toggle */
        if (_report_mode_enabled != 0) {
          _report_mode_enabled = 0;
          printf("report mode off");
        } else {
          _report_mode_enabled = 1;
          printf("report mode on\n");
        }
        break;
      case 'a': /* ADC raw data */
        printf("a=%d", adc_convert());
        break;
      case 'v': /* Version */
        printf_P(PSTR("\n"PACKAGE_STRING"\n"));
        break;
      case 't': /* temperature */
#ifdef SIMULATE_TEMP
        if (sscanf(buf, "%*s %d", &d) > 0) {
          printf("set temperature to %d", d);
          _fake_utophuile_oil_temperature = d;
        } else {
          printf("t=%d (sim)", utophuile_oil_temperature());
        }
#else /* SIMULATE_TEMP */
        printf("t=%d", utophuile_oil_temperature());
#endif /* SIMULATE_TEMP */
        break;
    }
    if (_report_mode_enabled == 0) printf("\n$ ");
    sleep_mode();
  }
  return (0);
}

static uint8_t _previous_utophuile_oil_temperature = 20;

uint8_t
utophuile_oil_temperature(void)
{
#ifndef SIMULATE_TEMP
  const int8_t temp = 185 - (adc_convert() / 2);
  uint8_t mean = (_previous_utophuile_oil_temperature + (uint8_t) temp);
// 	return ( uint8_t ) temp;
  mean /= 2;
  mean = (_previous_utophuile_oil_temperature + mean);
  mean /= 2;
  _previous_utophuile_oil_temperature = mean;
  return mean;
#else
  return _fake_utophuile_oil_temperature;
#endif /* SIMULATE_TEMP */
}

static uint8_t _gauge_light_enabled = 0;
void
utophuile_update_gauge_light_color(uint8_t oil_temperature)
{
  if (_gauge_light_enabled != 0) {
    rgb_mode_t rgb_mode = RGB_BLUE;
    if (oil_temperature >= GAUGE_TEMP_STAGE1) {
      rgb_mode = RGB_GREEN;
    }
    if (oil_temperature >= GAUGE_TEMP_STAGE2) {
      rgb_mode = RGB_YELLOW;
    }
    if (oil_temperature >= GAUGE_TEMP_STAGE3) {
      rgb_mode = RGB_RED;
    }
    rgb_set(rgb_mode);
  }
}

void
utophuile_toggle_gauge_light(void)
{
// 	printf_P(PSTR("gauge light toggled"));
  if (_gauge_light_enabled != 0) {
    rgb_set(RGB_OFF);
    _gauge_light_enabled = 0;
  } else {
    _gauge_light_enabled = 1;
  }
}

void
utophuile_set_mode(utophuile_mode_t mode)
{
  if (_utophuile_mode != mode) {
    _utophuile_previous_mode = _utophuile_mode;
    _utophuile_mode = mode;
    switch (mode) {
      case UTOPHUILE_MODE_OFF:
        relay_set(RELAY_OFF);
        leds_set(LED_ALL_OFF);
        break;
      case UTOPHUILE_MODE_HEATING:
        relay_set(_BV(RELAY_PUMP) | _BV(RELAY_HEATER));
        leds_set(LED_RED_ON);
        break;
      case UTOPHUILE_MODE_READY:
        relay_set(_BV(RELAY_PUMP) | _BV(RELAY_HEATER));
        leds_set(LED_ORANGE_BLINK);
        break;
      case UTOPHUILE_MODE_OIL:
        relay_set(_BV(RELAY_VALVE_INPUT) | _BV(RELAY_VALVE_OUTPUT) | _BV(RELAY_PUMP) | _BV(RELAY_HEATER));
        leds_set(LED_GREEN_ON);
        break;
      case UTOPHUILE_MODE_EMERGENCY:
        relay_set(RELAY_OFF);
        leds_set(LED_RED_BLINK);
        _utophuile_alerter_mode = UTOPHUILE_ALERTER_ENABLED;
        beep_play_partition_P(PSTR("G"));
        break;
      case UTOPHUILE_MODE_ERROR:
        leds_set(LED_ALL_BLINK);
        _utophuile_alerter_mode = UTOPHUILE_ALERTER_ENABLED;
        break;
    }
  }
}

// extern volatile twi_connection_state_t twi_connection_state;
void
utophuile_process(void)
{
  uint8_t oil_temperature = 0;

  const button_action_t requested_action = buttons_get_requested_action();

  // Retrieve temperature
  oil_temperature = utophuile_oil_temperature();

  // Print data if report mode is enabled
  if (_report_mode_enabled != 0) {
    printf("t=%d\n", oil_temperature);
  }

  /* Start / Stop actions */
  if (_utophuile_mode != UTOPHUILE_MODE_OFF) {
    if (twi_connection_state != TWI_CONNECTION_OK) {
      utophuile_set_mode(UTOPHUILE_MODE_ERROR);
    }

    if (requested_action == BUTTON_ACTION_POWER) {
      utophuile_set_mode(UTOPHUILE_MODE_OFF);
      beep_play_partition_P(PSTR("CB"));
    }
  } else {
    if (requested_action == BUTTON_ACTION_POWER) {
      utophuile_set_mode(UTOPHUILE_MODE_HEATING);
      beep_play_partition_P(PSTR("BC"));
    }
  }

  /* Gauge light actions */
  if (requested_action == BUTTON_ACTION_LIGHT) {
    utophuile_toggle_gauge_light();
  }
  utophuile_update_gauge_light_color(oil_temperature);

  switch (_utophuile_mode) {
    case UTOPHUILE_MODE_OFF:
      // Nothing to do
      break;
    case UTOPHUILE_MODE_HEATING:
//    wait for a correct oil temperature
      if (oil_temperature > UTOPHUILE_MIN_OIL_TEMPERATURE + UTOPHUILE_TOLERENCE_OIL_TEMPERATURE) {
        utophuile_set_mode(UTOPHUILE_MODE_READY);
        beep_play_partition_P(PSTR("F_F_F"));
      } else if (oil_temperature > UTOPHUILE_MAX_OIL_TEMPERATURE) {
        utophuile_set_mode(UTOPHUILE_MODE_EMERGENCY);
      }
      break;
    case UTOPHUILE_MODE_READY:
      if (oil_temperature < UTOPHUILE_MIN_OIL_TEMPERATURE) {
        utophuile_set_mode(UTOPHUILE_MODE_HEATING);
      } else if (oil_temperature > UTOPHUILE_MAX_OIL_TEMPERATURE) {
        utophuile_set_mode(UTOPHUILE_MODE_EMERGENCY);
      } else if (requested_action == BUTTON_ACTION_OK) {
//     wait for oil button
        utophuile_set_mode(UTOPHUILE_MODE_OIL);
        beep_play_partition_P(PSTR("F"));
      }
      break;
    case UTOPHUILE_MODE_OIL:
      if (oil_temperature < UTOPHUILE_MIN_OIL_TEMPERATURE) {
        utophuile_set_mode(UTOPHUILE_MODE_HEATING);
      } else if (oil_temperature > UTOPHUILE_MAX_OIL_TEMPERATURE) {
        utophuile_set_mode(UTOPHUILE_MODE_EMERGENCY);
      } else if (requested_action == BUTTON_ACTION_OK) {
        utophuile_set_mode(UTOPHUILE_MODE_READY);
        beep_play_partition_P(PSTR("E"));
      }
      break;
    case UTOPHUILE_MODE_EMERGENCY:
      if (oil_temperature < UTOPHUILE_MAX_OIL_TEMPERATURE - UTOPHUILE_TOLERENCE_OIL_TEMPERATURE) {
        utophuile_set_mode(UTOPHUILE_MODE_READY);
        beep_play_partition_P(PSTR("E"));
      } else if (requested_action == BUTTON_ACTION_OK) {
        _utophuile_alerter_mode = UTOPHUILE_ALERTER_DISABLED;
      } else if (_utophuile_alerter_mode == UTOPHUILE_ALERTER_ENABLED) {
        beep_play_partition_P(PSTR("G"));
      }
      break;
    case UTOPHUILE_MODE_ERROR:
      // wait for off button
      if (twi_connection_state == TWI_CONNECTION_OK) {
        utophuile_set_mode(_utophuile_previous_mode);
      } else if (requested_action == BUTTON_ACTION_OK) {
        _utophuile_alerter_mode = UTOPHUILE_ALERTER_DISABLED;
      } else if (_utophuile_alerter_mode == UTOPHUILE_ALERTER_ENABLED) {
        beep_play_partition_P(PSTR("GFG"));
      }
      break;
  }
}

