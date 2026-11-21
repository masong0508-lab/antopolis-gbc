/* The GBC difficulty and perk table lives in ROM bank 3, not the
   critical fixed 32KB. Caller hands a ten-byte temporary array:
   [mana_start,trk,drn,hcost0,hcost1,thr0,thr1,qmax0,qmax1,fcost].
   This pure helper does not call any GBDK services, allocate memory,
   consume RNG, touch sound or change multiplayer semantics. */
#ifndef ANTOPOLIS_RULES_CALC_H
#define ANTOPOLIS_RULES_CALC_H
#include <stdint.h>
static void rules_compute(uint8_t difficulty,uint8_t owned,uint8_t out[10]) {
  static const uint8_t rules[3][7] = {
    {14,15,63,4,12,7,4},
    {10,31,31,3, 8,5,5},
    { 6,63,15,2, 6,4,6}
  };
  const uint8_t *d=rules[difficulty < 3u ? difficulty : 1u];
  out[0]=d[0]; out[1]=d[1]; out[2]=d[2];
  out[3]=3u; out[4]=d[3];
  out[5]=8u; out[6]=d[4];
  out[7]=d[5]; out[8]=d[6]; out[9]=8u;
  if(owned & 1u) out[9]=4u;
  if(owned & 2u) out[3]=2u;
  if(owned & 4u) out[1]>>=1;
  if(owned & 8u) out[2]=(uint8_t)((out[2]<<1)|1u);
  if(owned & 16u) ++out[7];
  if(owned & 32u) ++out[4];
}
#endif
