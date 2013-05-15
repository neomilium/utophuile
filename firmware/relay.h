#ifndef __RELAY_H__
#define __RELAY_H__

#include <stdint.h>

#define RELAY_OFF	0
#define RELAY_VALVE_INPUT	4
#define RELAY_VALVE_OUTPUT	5
#define RELAY_PUMP		6
#define RELAY_HEATER		7

typedef enum {
  RELAY_CONNECTION_BROKEN,
  RELAY_CONNECTION_OK
} relay_connection_state_t;

volatile relay_connection_state_t relay_connection_state;

void relay_init(void);
void relay_set(const uint8_t relay_mode);

#endif /* __RELAY_H__ */
