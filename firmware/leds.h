#ifndef __LEDS_H__
#define __LEDS_H__

typedef enum {	LED_ALL_OFF = 0x00,
		LED_ALL_BLINK = 0x80,
		LED_GREEN_ON = 0x01,
		LED_GREEN_BLINK = 0x81,
		LED_ORANGE_ON = 0x02,
		LED_ORANGE_BLINK = 0x82,
		LED_RED_ON = 0x04,
		LED_RED_BLINK = 0x84
} leds_mode_t;

void leds_init(void);
void leds_set(const leds_mode_t mode);

#endif				/* !__LEDS_H__ */
