#include "scheduler.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include "scheduler.h"

#define SCHEDULER_MAX_HOOK_FCT		10
#define SCHEDULER_TCNT (0xFFFF - 31250) // Interrupt occurs (8 000 000 / 256) / 31250 = 1 hz

void scheduler_process_hooks(void);

typedef void (*_scheduler_hook_fct)(void);

static volatile uint8_t	_scheduler_hook_fct_count = 0;
static volatile _scheduler_hook_fct _scheduler_hook_fcts[SCHEDULER_MAX_HOOK_FCT];

ISR(TIMER1_OVF_vect)
{
  TCNT1 = SCHEDULER_TCNT;
  scheduler_process_hooks();
}

void
scheduler_init(void)
{
  TCCR1B = _BV(CS12);    //	// Timer1 clock use prescaled clock at clk/256 (i.e. 8 000 000 / 256 = 31250 hz)

  TCNT1 = SCHEDULER_TCNT;

  TIMSK |= _BV(TOIE1);	/* Enable interrupt */
}

void
scheduler_add_hook_fct(void (*fct)(void))
{
  _scheduler_hook_fcts[_scheduler_hook_fct_count++] = fct;
}


void
scheduler_process_hooks(void)
{
  for (uint8_t i = 0; i < _scheduler_hook_fct_count; i++) {
    _scheduler_hook_fcts[i]();
  }
}


