#include "relay.h"

#include "twi.h"
#include "scheduler.h"

#define PCF_ADDRESS 0x40

#include <stdio.h>
#include <avr/pgmspace.h>

static volatile uint8_t _relay_mode;

void relay_process(void);

void
relay_init(void)
{
  relay_connection_state = CONNECTION_OK;
  scheduler_add_hook_fct(relay_process);
}

void
relay_set_mode(const uint8_t relay_mode)
{
  // We keep 4 inputs bits (lsb) as it: these pins are inputs (feedback)
  _relay_mode = (relay_mode & 0xf0) | (_relay_mode & 0x0f);
}

void
relay_set(const uint8_t relay, const bool on)
{
  if (on)
    _relay_mode |= _BV(relay);
  else
    _relay_mode &= ~(_BV(relay));
}

uint8_t
relay_mode()
{
  return _relay_mode;
}

void
relay_process(void)
{
  // Inputs must be HIGH to be read
  uint8_t pcf_data = (~_relay_mode) | 0x0f;
  // Write outputs
  if (-1 == twi_write_bytes(PCF_ADDRESS, 1, &pcf_data)) {
    relay_connection_state = CONNECTION_BROKEN;
  } else {
    relay_connection_state = CONNECTION_OK;
    // Read inputs
    if (-1 == twi_read_bytes(PCF_ADDRESS, 1, &pcf_data)) {
      relay_connection_state = CONNECTION_BROKEN;
    } else {
      relay_connection_state = CONNECTION_OK;
      _relay_mode = (_relay_mode & 0xf0) | (pcf_data & 0x0f);
    }
  }
}
