/* One-time GBC difficulty/perk calculation; isolated in spacious
   switchable ROM3. The bank-1 sound ISR is disabled by the caller. */
#pragma bank 3
#include <gb/gb.h>
#include <stdint.h>
#include "rules_calc.h"
void rules_build(uint8_t difficulty,uint8_t owned,uint8_t out[10]) BANKED {
  rules_compute(difficulty,owned,out);
}
