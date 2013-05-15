#ifndef __BIT_FIELD_H__
#define __BIT_FIELD_H__


// Define the bits in the port
typedef struct {
  unsigned char bit0 : 1,
           bit1 : 1,
           bit2 : 1,
           bit3 : 1,
           bit4 : 1,
           bit5 : 1,
           bit6 : 1,
           bit7 : 1;
} bit_field;

// Define macro to get the value of each bit
#define GET_BIT(port) (*(volatile bit_field *) (_SFR_ADDR(port)))

#endif				/* !__BIT_FIELD_H__ */