#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#include "utophuile.h"

#include "ads1115.h"
#include "leds.h"
#include "beep.h"
#include "buttons.h"
#include "uart.h"
#include "twi.h"
#include "relay.h"
#include "shell.h"

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

// Shell commands
shell_command_t shell_commands[SHELL_COMMAND_COUNT];
void utophuile_command_help(const char *args);
void utophuile_command_status(const char *args);
void utophuile_debug_command_pcf(const char *args);
void utophuile_debug_command_monitor(const char *args);
void utophuile_debug_command_fake(const char *args);


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

static uint8_t _report_mode_enabled = 0;
static bool _debug_mode = true;

// Is oil temperature a fake ? (ie. sets by user in debug mode)
static bool _utophuile_oil_temperature_is_fake = false;

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

  // Buzzer
  beep_init();

  // I²C / TWI
  twi_init();

  // 16bits Analog-to-Digital Converter
  ads1115_init();

//  relay_init();

  scheduler_add_hook_fct(utophuile_process);

  SHELL_COMMAND_DECL(0, "help", "this help", false, utophuile_command_help);
  SHELL_COMMAND_DECL(1, "status", "system status", false, utophuile_command_status);
  SHELL_COMMAND_DECL(2, "pcf", "read/write from/to PCF8574 (relays)", true, utophuile_debug_command_pcf);
  SHELL_COMMAND_DECL(3, "monitor", "enable monitor mode", true, utophuile_debug_command_monitor);
  SHELL_COMMAND_DECL(4, "fake", "set a simulated value", true, utophuile_debug_command_fake);

  sei();   /* Enable interrupts */

  utophuile_set_mode(UTOPHUILE_MODE_OFF);

  for (;;) {
    shell_loop();
    /*
          case 'v': // Version
            printf_P(PSTR("\n"PACKAGE_STRING"\n"));
            break;
        }
    */
    sleep_mode();
  }
  return (0);
}


static int16_t _fake_oil_temperature;

int16_t
utophuile_oil_temperature(void)
{
  if(!_utophuile_oil_temperature_is_fake) {
    int16_t adc = ads1115_read();
    // return (((double)adc * 0.0217220010422) - 259.74025974);
    // printf_P(PSTR("adc: %"PRIi16), adc);
    adc /= 46; // * 0.0217220010422 => / 46.0362743772
    // printf_P(PSTR(", tmp: %"PRIi16), adc);
    adc -= 259; // - 259.74025974
    // printf_P(PSTR(", res: %"PRIi16"\n"), adc);
    return adc;
  } else {
    return _fake_oil_temperature;
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

static int16_t _utophuile_oil_temperature = 20.0;

void
utophuile_process(void)
{
  const button_action_t requested_action = buttons_get_requested_action();

  // Retrieve temperature
  _utophuile_oil_temperature = utophuile_oil_temperature();

  // Print data if report mode is enabled
  if (_report_mode_enabled != 0) {
    printf("t=%"PRIi16"\n", _utophuile_oil_temperature);
  }

  static twi_connection_state local = CONNECTION_OK;
  if ((ads1115_connection_state != local)) {
    if (ads1115_connection_state == CONNECTION_OK) {
      beep_play_partition_P(PSTR("DAA"));
    } else {
      beep_play_partition_P(PSTR("ADD"));
    }
    local = ads1115_connection_state;
  }

  /* Start / Stop actions */
  if (_utophuile_mode != UTOPHUILE_MODE_OFF) {
    // FIXME Use different error beeps
    if ((relay_connection_state != CONNECTION_OK) || (ads1115_connection_state != CONNECTION_OK)) {
      utophuile_set_mode(UTOPHUILE_MODE_ERROR);
    }

    if (requested_action == BUTTON_ACTION_POWER) {
      utophuile_set_mode(UTOPHUILE_MODE_OFF);
      beep_play_partition_P(PSTR("Cb"));
    }
  } else {
    if (requested_action == BUTTON_ACTION_POWER) {
      utophuile_set_mode(UTOPHUILE_MODE_HEATING);
      beep_play_partition_P(PSTR("bC"));
    }
  }

  switch (_utophuile_mode) {
    case UTOPHUILE_MODE_OFF:
      // Nothing to do
      break;
    case UTOPHUILE_MODE_HEATING:
      // Wait for a correct oil temperature
      if (_utophuile_oil_temperature > UTOPHUILE_MIN_OIL_TEMPERATURE + UTOPHUILE_TOLERENCE_OIL_TEMPERATURE) {
        utophuile_set_mode(UTOPHUILE_MODE_READY);
        beep_play_partition_P(PSTR("F_F_F"));
      } else if (_utophuile_oil_temperature > UTOPHUILE_MAX_OIL_TEMPERATURE) {
        utophuile_set_mode(UTOPHUILE_MODE_EMERGENCY);
      }
      break;
    case UTOPHUILE_MODE_READY:
      // Wait for oil mode request while checking temperatures stay OK
      if (_utophuile_oil_temperature < UTOPHUILE_MIN_OIL_TEMPERATURE) {
        utophuile_set_mode(UTOPHUILE_MODE_HEATING);
      } else if (_utophuile_oil_temperature > UTOPHUILE_MAX_OIL_TEMPERATURE) {
        utophuile_set_mode(UTOPHUILE_MODE_EMERGENCY);
      } else if (requested_action == BUTTON_ACTION_OK) {
        // User ask for oil mode
        utophuile_set_mode(UTOPHUILE_MODE_OIL);
        beep_play_partition_P(PSTR("F"));
      }
      break;
    case UTOPHUILE_MODE_OIL:
      // Check if all is OK
      if (_utophuile_oil_temperature < UTOPHUILE_MIN_OIL_TEMPERATURE) {
        utophuile_set_mode(UTOPHUILE_MODE_HEATING);
      } else if (_utophuile_oil_temperature > UTOPHUILE_MAX_OIL_TEMPERATURE) {
        utophuile_set_mode(UTOPHUILE_MODE_EMERGENCY);
      } else if (requested_action == BUTTON_ACTION_OK) {
        // User want to stop oil usage
        utophuile_set_mode(UTOPHUILE_MODE_READY);
        beep_play_partition_P(PSTR("E"));
      }
      break;
    case UTOPHUILE_MODE_EMERGENCY:
      // Waiting for lower oil temperature
      if (_utophuile_oil_temperature < UTOPHUILE_MAX_OIL_TEMPERATURE - UTOPHUILE_TOLERENCE_OIL_TEMPERATURE) {
        utophuile_set_mode(UTOPHUILE_MODE_READY);
        beep_play_partition_P(PSTR("E"));
      } else if (requested_action == BUTTON_ACTION_OK) {
        // User want to stop beeps :)
        _utophuile_alerter_mode = UTOPHUILE_ALERTER_DISABLED;
      } else if (_utophuile_alerter_mode == UTOPHUILE_ALERTER_ENABLED) {
        beep_play_partition_P(PSTR("G"));
      }
      break;
    case UTOPHUILE_MODE_ERROR:
      // Something went wrong, checking if all is back to normal or emit repeated beeps
      if (relay_connection_state == CONNECTION_OK) {
        // Back to normal
        utophuile_set_mode(_utophuile_previous_mode);
      } else if (requested_action == BUTTON_ACTION_OK) {
        // User want to stop beeps :)
        _utophuile_alerter_mode = UTOPHUILE_ALERTER_DISABLED;
      } else if (_utophuile_alerter_mode == UTOPHUILE_ALERTER_ENABLED) {
        beep_play_partition_P(PSTR("GFG"));
      }
      break;
  }
}

// Help command
void
utophuile_command_help(const char *args)
{
  (void)args;
  printf_P(PSTR("supported commands:\n"));
  for (size_t n = 0; n < SHELL_COMMAND_COUNT; n++) {
    if ((_debug_mode) || (shell_commands[n].debug == false)) {
      printf_P(PSTR("  %S - %S\n"), shell_commands[n].text, shell_commands[n].description);
    }
  }
}

// Status command
void
utophuile_command_status(const char *args)
{
  (void)args;

  // Mode
  switch (_utophuile_mode) {
    case UTOPHUILE_MODE_OFF:
      printf_P(PSTR("Status: OFF\n"));
      break;
    case UTOPHUILE_MODE_HEATING:
      printf_P(PSTR("Status: HEATING\n"));
      break;
    case UTOPHUILE_MODE_READY:
      printf_P(PSTR("Status: READY\n"));
      break;
    case UTOPHUILE_MODE_OIL:
      printf_P(PSTR("Status: OIL\n"));
      break;
    case UTOPHUILE_MODE_EMERGENCY:
      printf_P(PSTR("Status: EMERGENCY\n"));
      break;
    case UTOPHUILE_MODE_ERROR:
      printf_P(PSTR("Status: ERROR\n"));
      break;
  }

  // Temperature
  printf_P(PSTR("Temperature: %"PRIi16" °C %s\n"), _utophuile_oil_temperature, _utophuile_oil_temperature_is_fake ? " (fake)" : "");

  // Relays
  const uint8_t rm = relay_mode();
  printf_P(PSTR("Valve input: %s (feedback: %s)\n"),	(rm & _BV(RELAY_VALVE_INPUT)) ? "ON" : "OFF", (rm & _BV(RELAY_FB_VALVE_INPUT)) ? "ON" : "OFF");
  printf_P(PSTR("Valve output: %s (feedback: %s)\n"),	(rm & _BV(RELAY_VALVE_OUTPUT)) ? "ON" : "OFF", (rm & _BV(RELAY_FB_VALVE_OUTPUT)) ? "ON" : "OFF");
  printf_P(PSTR("Pump: %s (feedback: %s)\n"),	(rm & _BV(RELAY_PUMP)) ? "ON" : "OFF", (rm & _BV(RELAY_FB_PUMP)) ? "ON" : "OFF");
  printf_P(PSTR("Heater: %s (feedback: %s)\n"),	(rm & _BV(RELAY_HEATER)) ? "ON" : "OFF", (rm & _BV(RELAY_FB_HEATER)) ? "ON" : "OFF");
}

// PCF debug command
void
utophuile_debug_command_pcf(const char *args)
{
  uint8_t pcf_data;
  if (sscanf_P(args, PSTR("%*s %" PRIu8), &pcf_data) > 0) {
    if (-1 != twi_write_bytes(0x40, 1, &pcf_data)) {
      printf_P(PSTR("write %#02x to pcf data\n"), pcf_data);
    } else {
      printf_P(PSTR("unable to write to pcf\n"));
    }
  } else {
    printf_P(PSTR("Reading PCF value...\n"));
    if (-1 != twi_read_bytes(0x40, 1, &pcf_data)) {
      printf_P(PSTR("pcf data = %#02x\n"), pcf_data);
    } else {
      printf_P(PSTR("unable to read pcf data\n"));
    }
  }
}

// Monitor debug command
void
utophuile_debug_command_monitor(const char *args)
{
  (void)args;
  if (_report_mode_enabled) {
    printf_P(PSTR("monitor mode off\n"));
    _report_mode_enabled = 0;
  } else {
    printf_P(PSTR("monitor mode on\n"));
    _report_mode_enabled = 1;
  }
}

// Fake values
void
utophuile_debug_command_fake(const char *args)
{
  char subcommand[128];
  if (sscanf_P(args, PSTR("%*s %s"), subcommand) > 0) {
    // Look for an exact match
    for (size_t n = 0; n < 1; n++) {
      if (0 == strcmp_P(subcommand, PSTR("temp"))) {
        if (sscanf_P(args, PSTR("%*s %*s %"PRIi16), &_fake_oil_temperature) > 0) {
          _utophuile_oil_temperature_is_fake = true;
          printf_P(PSTR("fake oil temperature: %"PRIi16"\n"), _fake_oil_temperature);
          return;
        } else {
          _utophuile_oil_temperature_is_fake = false;
          printf_P(PSTR("fake oil temperature disabled\n"));
          return;
        }
      }
    }
    printf_P(PSTR("\"%s\" is not an unknown subcommand of 'fake'\n"), subcommand);
  }
}
