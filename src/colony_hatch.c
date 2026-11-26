/* Single-pass hatching in ROM3. Pointer increments avoid a runtime
   index multiplication that could call a helper in the unmapped bank.
   Caller masks audio interrupts during the banked call. */
#pragma bank 3
#include <gb/gb.h>
#include <stdint.h>
#include "eye_occupancy.h"

extern EyeAnt ant[EYE_OCC_ANTS];
extern uint8_t nestx[2], nesty[2];

uint8_t colony_spawn(uint8_t team, uint8_t counters[2]) BANKED {
  uint8_t i, n = 0;
  EyeAnt *p = ant, *slot = 0;
  for (i = 0; i < EYE_OCC_ANTS; ++i, ++p) {
    if (p->alive) {
      if (p->team == team && ++n >= EYE_OCC_ANTS / 2u) return 0;
    } else if (!slot) slot = p;
  }
  if (!slot) return 0;
  slot->x = nestx[team]; slot->y = nesty[team];
  slot->team = team; slot->alive = 1; slot->carry = 0;
  slot->sol = ((counters[team]++ & 3u) == 0u);
  return 1;
}
