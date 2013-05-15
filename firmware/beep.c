#include "beep.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include <string.h>

#define TIMER0_PWM_INIT		_BV(WGM01) | _BV(COM00) | _BV(CS01) | _BV(CS00)	/* 8 Mhz / 64 */

#define OCR			OCR0
#define DDROC			DDRB
#define OC0			PB3

#include "leds.h"

#define BEEP_DO3		238
#define BEEP_RE3		212
#define BEEP_MI3		189
#define BEEP_FA3		178
#define BEEP_SOL3		159
#define BEEP_LA3		142
#define BEEP_SI3		126

/*
	fq = 1 / (x * 1/125k * 2)
	x = 125k / ( 2 * fq )
*/
#define FQ2CTC(fq) ( 125000 / ( 2 * fq ) )
typedef enum {
  NOTE_DO2 = FQ2CTC(132),
  NOTE_RE2 = FQ2CTC(148),
  NOTE_MI2 = FQ2CTC(165),
  NOTE_FA2 = FQ2CTC(176),
  NOTE_SOL2 = FQ2CTC(198),
  NOTE_LA2 = FQ2CTC(220),
  NOTE_SI2 = FQ2CTC(247),
  NOTE_DO3 = FQ2CTC(264),
  NOTE_RE3 = FQ2CTC(297),
  NOTE_MI3 = FQ2CTC(330),
  NOTE_FA3 = FQ2CTC(352),
  NOTE_SOL3 = FQ2CTC(396),
  NOTE_LA3 = FQ2CTC(440),
  NOTE_SI3 = FQ2CTC(495),
  NOTE_DO4 = FQ2CTC(528),
  NOTE_RE4 = FQ2CTC(594),
  NOTE_MI4 = FQ2CTC(660),
  NOTE_FA4 = FQ2CTC(704),
  NOTE_SOL4 = FQ2CTC(792),
  NOTE_LA4 = FQ2CTC(880),
  NOTE_SI4 = FQ2CTC(990)
} note_t;


// ISR (TIMER0_COMP_vect)
// {
// 	leds_set(LED_GREEN_BLINK);
// }

void beep_ctc_start(void);
void beep_ctc_stop(void);

void
beep_play_note(const note_t note)
{
  OCR = note;
  beep_ctc_start();
  _delay_ms(150.);
  beep_ctc_stop();
}

void beep_play_partition_P(char *partition)
{
  char	n;
  while ((n = pgm_read_byte(partition++))) {
    switch (n) {
      case 'c': 		// Do = C
        beep_play_note(NOTE_DO3);
        break;
      case 'd': 		// Ré = D
        beep_play_note(NOTE_RE3);
        break;
      case 'e': 		// Mi = E
        beep_play_note(NOTE_MI3);
        break;
      case 'f': 		// Fa = F
        beep_play_note(NOTE_FA3);
        break;
      case 'g': 		// Sol = G
        beep_play_note(NOTE_SOL3);
        break;
      case 'A': 		// La = A
        beep_play_note(NOTE_LA3);
        break;
      case 'B': 		// Si3
        beep_play_note(NOTE_SI3);
        break;
      case 'C': 		// Do4
        beep_play_note(NOTE_DO4);
        break;
      case 'D': 		// Ré4
        beep_play_note(NOTE_RE4);
        break;
      case 'E': 		// Mi4
        beep_play_note(NOTE_MI4);
        break;
      case 'F': 		// Fa4
        beep_play_note(NOTE_FA4);
        break;
      case 'G': 		// Sol4
        beep_play_note(NOTE_SOL4);
        break;
      default:
        _delay_ms(75.);
    }
  }
}

void
beep_init(void)
{
  // Au clair de la lune
// 	beep_play_partition("c_c_c_d_e__d__c_e_d_d_c");

  // Hymne à la joie
// 	beep_play_partition("e_e_f_g_g_f_e_d_c_c_d_e_e_d_d_e_e_f_g_g_f_e_d_c_c_d_e_d_c_c_d_d_f_c_d_e_f_e_c_d_e_f_e_d_c_d_g_");
}

void
beep_ctc_start(void)
{
  /* Enable OC0 as output. */
  DDROC |= _BV(OC0);

  /* Timer 0 is 8-bit PWM. */
  TCCR0 = TIMER0_PWM_INIT;

  /* Enable timer 0 overflow interrupt. */
// 	TIMSK |= _BV(OCIE0);
}

void
beep_ctc_stop(void)
{
  /* Timer 0 is 8-bit PWM. */
  TCCR0 = 0x00;

  /* Disable timer 2 overflow interrupt. */
// 	TIMSK &= ~(_BV(OCIE0));
}
