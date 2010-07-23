#include "relay.h"

#include "twi.h"
#include "scheduler.h"

static volatile uint8_t _relay_mode;

void relay_process(void);

void
relay_init(void)
{
	twi_connection_state = TWI_CONNECTION_OK;
	scheduler_add_hook_fct( relay_process );
}

void
relay_set(const uint8_t relay_mode )
{
	_relay_mode = relay_mode;
}

void
relay_process(void)
{
	uint8_t pcf_data = (~_relay_mode) | 0x0F;
	if (-1 == twi_write_bytes ( 0x40, 1, &pcf_data )) {
		twi_connection_state = TWI_CONNECTION_BROKEN;
	} else {
		twi_connection_state = TWI_CONNECTION_OK;
	}
}