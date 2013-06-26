#include "beep.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include <string.h>

#define OCR			OCR0A
#define DDROC			DDRD
#define OC0			PD6	// Arduino Digital Pin 6
#define TCCRA			TCCR0A
#define TCCRB			TCCR0B

#include "leds.h"

/*
  PWM is used in CTC mode @62.5Khz (16Mhz / 256)

  fq = 1 / (x * 1/62.5k * 2)
  x = 62.5k / ( 2 * fq )

  http://www.phy.mtu.edu/~suits/notefreqs.html
*/
#define FQ2CTC(fq) ( 62500 / ( 2 * fq ) )
typedef enum {
  NOTE_DO2 = FQ2CTC(131),	// C3
  NOTE_RE2 = FQ2CTC(147),	// D3
  NOTE_MI2 = FQ2CTC(165),	// E3
  NOTE_FA2 = FQ2CTC(175),	// F3
  NOTE_SOL2 = FQ2CTC(196),	// G3
  NOTE_LA2 = FQ2CTC(220),	// A3
  NOTE_SI2 = FQ2CTC(247),	// B3
  NOTE_DO3 = FQ2CTC(262),	// C4
  NOTE_RE3 = FQ2CTC(294),	// D4
  NOTE_MI3 = FQ2CTC(330),	// E4
  NOTE_FA3 = FQ2CTC(349),	// F4
  NOTE_SOL3 = FQ2CTC(392),	// G4
  NOTE_LA3 = FQ2CTC(440),	// A4
  NOTE_SI3 = FQ2CTC(494),	// B4
  NOTE_DO4 = FQ2CTC(523),	// C5
  NOTE_RE4 = FQ2CTC(587),	// D5
  NOTE_MI4 = FQ2CTC(659),	// E5
  NOTE_FA4 = FQ2CTC(698),	// F5
  NOTE_SOL4 = FQ2CTC(784),	// G5
  NOTE_LA4 = FQ2CTC(880),	// A5
  NOTE_SI4 = FQ2CTC(988)	// B5
} note_t;


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

void beep_play_partition_P(const char *partition)
{
  char	n;
  while ((n = pgm_read_byte(partition++))) {
    switch (n) {
      case 'c': 		// Do = C4
        beep_play_note(NOTE_DO3);
        break;
      case 'd': 		// Ré = D4
        beep_play_note(NOTE_RE3);
        break;
      case 'e': 		// Mi = E4
        beep_play_note(NOTE_MI3);
        break;
      case 'f': 		// Fa = F4
        beep_play_note(NOTE_FA3);
        break;
      case 'g': 		// Sol = G4
        beep_play_note(NOTE_SOL3);
        break;
      case 'a': 		// La = A4
        beep_play_note(NOTE_LA3);
        break;
      case 'b': 		// Si3 = B4
        beep_play_note(NOTE_SI3);
        break;
      case 'C': 		// Do4 = C5
        beep_play_note(NOTE_DO4);
        break;
      case 'D': 		// Ré4 = D5
        beep_play_note(NOTE_RE4);
        break;
      case 'E': 		// Mi4 = E5
        beep_play_note(NOTE_MI4);
        break;
      case 'F': 		// Fa4 = F5
        beep_play_note(NOTE_FA4);
        break;
      case 'G': 		// Sol4 = G5
        beep_play_note(NOTE_SOL4);
        break;
      case 'A': 		// La4 = A5
        beep_play_note(NOTE_LA4);
        break;
      case 'B': 		// Si4 = B5
        beep_play_note(NOTE_SI4);
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
//  beep_play_partition_P(PSTR("c_c_c_d_e__d__c_e_d_d_c"));
//  beep_play_partition_P(PSTR("C_C_C_D_E__D__C_E_D_D_C"));

  // Hymne à la joie
//  beep_play_partition_P(PSTR("e_e_f_g_g_f_e_d_c_c_d_e_e_d_d_e_e_f_g_g_f_e_d_c_c_d_e_d_c_c_d_d_f_c_d_e_f_e_c_d_e_f_e_d_c_d_g_"));

  // Utop'huile init beeps :)
  beep_play_partition_P(PSTR("GA_AG"));
}

void
beep_ctc_start(void)
{
  /* Enable OC0 as output. */
  DDROC |= _BV(OC0);

  /* Timer 0 is 8-bit PWM. */
  TCCRA = _BV(WGM01) | _BV(COM0A0);	/*  Compare Output Mode, Fast PWM Mode + CTC mode */
  TCCRB = _BV(CS02);			/* 16 Mhz / 256 */

  /* Enable timer 0 overflow interrupt. */
// 	TIMSK |= _BV(OCIE0);
}

void
beep_ctc_stop(void)
{
  /* Timer 0 is 8-bit PWM. */
  TCCRA = 0x00;

  /* Disable timer 2 overflow interrupt. */
// 	TIMSK &= ~(_BV(OCIE0));
}
