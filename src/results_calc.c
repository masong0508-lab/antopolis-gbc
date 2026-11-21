/* End-of-match arithmetic runs only once, after the 2D/3D simulation
   stops. Store its code in the spacious bank 3 rather than the saturated
   fixed 32KB. The actual world clock, rewards and record format are
   bit-exact to the original implementation. No library division or
   16-bit multiplication is used: under GBDK those helpers live in
   switchable ROM1 and are unsafe while bank 3 is active.
   Caller guards interrupts around the BANKED function. */
#ifndef RESULTS_HOST_TEST
#pragma bank 3
#include <gb/gb.h>
#else
#define BANKED
#endif
#include <stdint.h>

void results_compute(uint16_t ticks, uint8_t difficulty,
                     uint8_t ants, uint8_t approval, uint8_t queen_hp,
                     uint16_t *seconds, uint16_t *score) BANKED {
  uint16_t secs=0, points;
  // Original: (uint32_t)ticks * 2 / 15, capped at 5999 and
  // with a one-second minimum. Ten 15-tick groups per iteration
  // keep end-of-game division below 301 cheap iterations.
  while(ticks>=150u) {
    ticks-=150u; secs+=20u;
    if(secs>=6000u) {
      *seconds=5999u;
      points=(uint16_t)(difficulty == 0u ? 300u :
                        difficulty == 1u ? 600u : 900u);
      points+=(uint16_t)((((uint16_t)ants)<<2)+ants+approval+
                         (((uint16_t)queen_hp)<<3)+
                         (((uint16_t)queen_hp)<<1));
      *score=points;
      return;
    }
  }
  while(ticks>=15u) { ticks-=15u; secs+=2u; }
  if(ticks>=8u) ++secs;
  if(!secs) secs=1u;
  *seconds=secs;

  // Original: 300 * (difficulty+1), a speed bonus below ten
  // minutes, 5 per surviving ant, approval, 10 per queen HP.
  points=(uint16_t)(difficulty == 0u ? 300u :
                    difficulty == 1u ? 600u : 900u);
  if(secs<600u) points+=(uint16_t)(600u-secs);
  points+=(uint16_t)((((uint16_t)ants)<<2)+ants+approval+
                     (((uint16_t)queen_hp)<<3)+
                     (((uint16_t)queen_hp)<<1));
  *score=points;
}
