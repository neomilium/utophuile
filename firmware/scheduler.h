#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <stdint.h>

void		scheduler_init  (void);
void		scheduler_add_hook_fct ( void ( *fct ) ( void ) );
#endif
