#ifndef __ADS1115_H__
#define __ADS1115_H__

#include "twi.h"

volatile twi_connection_state ads1115_connection_state;
volatile int ads1115_connection_last_error;

void ads1115_init(void);
int16_t ads1115_read(void);

#endif /* __ADS1115_H__ */
