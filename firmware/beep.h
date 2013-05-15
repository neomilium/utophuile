#ifndef __BEEP_H__
#define __BEEP_H__

#include <avr/pgmspace.h>

void beep_init(void);
void beep_play_partition_P(char *partition);

#endif				/* !__BEEP_H__ */
