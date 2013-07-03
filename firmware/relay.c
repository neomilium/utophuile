#include "relay.h"

#include "twi.h"
#include "scheduler.h"

#define PCF_ADDRESS 0x40

static volatile uint8_t _relay_mode;

void relay_process(void);

void
relay_init(void)
{
  relay_connection_state = CONNECTION_OK;
  scheduler_add_hook_fct(relay_process);
}

void
relay_set(const uint8_t relay_mode)
{
  _relay_mode = relay_mode;
}

void
relay_process(void)
{
  uint8_t pcf_data = (~_relay_mode) | 0x0F;
  if (-1 == twi_write_bytes(PCF_ADDRESS, 1, &pcf_data)) {
    relay_connection_state = CONNECTION_BROKEN;
  } else {
    relay_connection_state = CONNECTION_OK;
  }
}
