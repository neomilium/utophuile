#ifndef __BUTTONS_H__
#define __BUTTONS_H__

typedef enum { 
		BUTTON_ACTION_NONE,
		BUTTON_ACTION_OK,
		BUTTON_ACTION_POWER,
		BUTTON_ACTION_LIGHT,
} button_action_t;

void buttons_init(void);
button_action_t buttons_get_requested_action(void);

#endif	/*	__BUTTONS_H__ */

