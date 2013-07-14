#ifndef __RELAY_H__
#define __RELAY_H__

#include "twi.h"

// Relay board is wired around a PCF8574 chip.
// PCF8574 is an I²C 8bits input/output port
// In our design, we use 4 bits (msb) as outputs to set relay and the 4
// remaining bits (lsb) as an electrical feedback
#define RELAY_OFF	0
// Outputs
#define RELAY_VALVE_INPUT	4
#define RELAY_VALVE_OUTPUT	5
#define RELAY_PUMP		6
#define RELAY_HEATER		7
// Inputs
#define RELAY_FB_VALVE_INPUT	0
#define RELAY_FB_VALVE_OUTPUT	1
#define RELAY_FB_PUMP		2
#define RELAY_FB_HEATER		3

volatile twi_connection_state relay_connection_state;

void relay_init(void);
void relay_set(const uint8_t relay_mode);
uint8_t relay_mode(void);

#endif /* __RELAY_H__ */
