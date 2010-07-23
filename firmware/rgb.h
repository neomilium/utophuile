#ifndef __RGB_H__
#define __RGB_H__

typedef enum {
		RGB_OFF,
		RGB_BLUE,
		RGB_GREEN,
		RGB_YELLOW,
		RGB_RED,
} rgb_mode_t;

void rgb_init(void);
void rgb_set(const rgb_mode_t mode);

#endif				/* !__RGB_H__ */
