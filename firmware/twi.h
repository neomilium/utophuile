#ifndef __TWI_H__
#define __TWI_H__

#include <stdint.h>

void twi_init(void);
int twi_read_bytes(uint8_t addr, int len, uint8_t *buf);
int twi_write_bytes(uint8_t addr, int len, uint8_t *buf);

#endif				/* !__TWI_H__ */