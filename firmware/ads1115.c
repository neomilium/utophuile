#include "ads1115.h"

#include "twi.h"

#define ADS1115_ADDRESS (0x48<<1)

#include <stdio.h>
#include <avr/pgmspace.h>

#define OS 	15
#define MUXX2 	14
#define MUXX1 	13
#define MUXX0 	12
#define PGA2 	11
#define PGA1 	10
#define PGA0 	9
#define MODE 	8
#define DR2 	7
#define DR1 	6
#define DR0 	5
#define COMP_MODE 	4
#define COMP_POL 	3
#define COMP_LAT 	2
#define COMP_QUE1 	1
#define COMP_QUE0 	0

// Programmable gain amplifier (PGA) configuration
// 000 : FS = ±6.144V¹
#define PGA_6_144 		(0)
// 001 : FS = ±4.096V¹
#define PGA_4_096 		(_BV(PGA0))
// 010 : FS = ±2.048V (default)
#define PGA_2_048 		(_BV(PGA1))
// 011 : FS = ±1.024V
#define PGA_1_024 		(_BV(PGA1) | _BV(PGA0))
// 100 : FS = ±0.512V
#define PGA_0_512 		(_BV(PGA2))
// 101 : FS = ±0.256V
// 110 : FS = ±0.256V
// 111 : FS = ±0.256V
#define PGA_0_256		(_BV(PGA2) | _BV(PGA1) | _BV(PGA0))

// Data rate (DR)
// 000 : 8SPS
#define DR_8SPS 		(0)
// 100 : 128SPS (default)
#define DR_128SPS 		(_BV(DR2))

// Input multiplexer configuration
// 000 : AINP = AIN0 and AINN = AIN1 (default)
#define IM_IN0_IN1 		(0)
// 001 : AINP = AIN0 and AINN = AIN3
#define IM_IN0_IN3 		(_BV(MUXX0))
// 010 : AINP = AIN1 and AINN = AIN3
#define IM_IN1_IN3 		(_BV(MUXX1))
// 011 : AINP = AIN2 and AINN = AIN3
#define IM_IN2_IN3 		(_BV(MUXX1) | _BV(MUXX0))

// 100 : AINP = AIN0 and AINN = GND
#define IM_IN0_GND 		(_BV(MUXX2))
// 101 : AINP = AIN1 and AINN = GND
#define IM_IN1_GND 		(_BV(MUXX2) | _BV(MUXX0))
// 110 : AINP = AIN2 and AINN = GND
#define IM_IN2_GND 		(_BV(MUXX2) | _BV(MUXX1))
// 111 : AINP = AIN3 and AINN = GND
#define IM_IN3_GND 		(_BV(MUXX2) | _BV(MUXX1) | _BV(MUXX0))

#define ADS1115_CFG_CH0 	(_BV(OS) | IM_IN1_GND | _BV(MODE) | DR_8SPS | PGA_2_048 | _BV(COMP_QUE1) | _BV(COMP_QUE0))

#define ADS1115_REG_CONVERSION 	0x00
#define ADS1115_REG_CONFIG 	0x01

void
ads1115_init(void)
{
  // FIXME Test connection
  ads1115_connection_state = CONNECTION_OK;
  ads1115_connection_last_error = 0;
}

#define ADS1115_ERR_CONNECTION_LOST -32768
int16_t
ads1115_read(void)
{
  // Set configuration
  uint8_t data[3] = { ADS1115_REG_CONFIG, (ADS1115_CFG_CH0 >> 8), (ADS1115_CFG_CH0 & 0xff) };
  if (0 > (ads1115_connection_last_error = twi_write_bytes(ADS1115_ADDRESS, 3, data))) {
    ads1115_connection_state = CONNECTION_BROKEN;
    return ADS1115_ERR_CONNECTION_LOST;
  } else {
    ads1115_connection_state = CONNECTION_OK;
  }

  // Read configuration
  if (0 > twi_read_bytes(ADS1115_ADDRESS, 2, data)) {
    ads1115_connection_state = CONNECTION_BROKEN;
    return ADS1115_ERR_CONNECTION_LOST;
  } else {
    ads1115_connection_state = CONNECTION_OK;
  }

  // Compare if its OK
  uint16_t config = (data[0] << 8) | data[1];
  printf_P(PSTR("Expected: %04x, register: %04x\n"), ADS1115_CFG_CH0 & ~(0x8000), config);

  // Retrieve conversion result
  // Important: this will return the PREVIOUS converted value, to retrieve the
  // current shot we need to way OS flag is set on configuration register

  // Point to "conversion" register
  data[0] = ADS1115_REG_CONVERSION;
  if (0 > (ads1115_connection_last_error = twi_write_bytes(ADS1115_ADDRESS, 1, data))) {
    ads1115_connection_state = CONNECTION_BROKEN;
    return ADS1115_ERR_CONNECTION_LOST;
  } else {
    ads1115_connection_state = CONNECTION_OK;
  }

  // Read register
  if (0 > twi_read_bytes(ADS1115_ADDRESS, 2, data)) {
    ads1115_connection_state = CONNECTION_BROKEN;
    return ADS1115_ERR_CONNECTION_LOST;
  } else {
    ads1115_connection_state = CONNECTION_OK;
  }

  // Convert value to int16_t
  int16_t value = (data[0] << 8) | data[1];
  return value;
}

