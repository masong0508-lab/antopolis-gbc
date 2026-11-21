/* Compare ALL bank-3 difficulty/perk combinations to the legacy GBC
   set_rules behavior. Exact resource, popularity, hatching, soldier and
   queen thresholds matter for save compatibility and linked multiplayer. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/rules_calc.h"
int main(void) {
  static const uint8_t original[3][7] = {
    {14,15,63,4,12,7,4},
    {10,31,31,3,8,5,5},
    {6,63,15,2,6,4,6}
  };
  uint8_t out[10],expect[10],copy[10];
  for(uint8_t d=0;d<3;d++) for(uint8_t perk=0;perk<64;perk++) {
    const uint8_t *x=original[d];
    expect[0]=x[0]; expect[1]=x[1]; expect[2]=x[2];
    expect[3]=3; expect[4]=x[3]; expect[5]=8;expect[6]=x[4];
    expect[7]=x[5]; expect[8]=x[6]; expect[9]=8;
    if(perk&1) expect[9]=4;
    if(perk&2) expect[3]=2;
    if(perk&4) expect[1]>>=1;
    if(perk&8) expect[2]=(uint8_t)((expect[2]<<1)|1u);
    if(perk&16) expect[7]++;
    if(perk&32) expect[4]++;
    memset(out,0xAB,sizeof out);
    rules_compute(d,perk,out);
    assert(!memcmp(out,expect,sizeof out));
    memcpy(copy,out,sizeof out);
    rules_compute(d,perk,out);
    assert(!memcmp(out,copy,sizeof out));
  }
  // Corrupted title settings previously clamp back to NORMAL.
  rules_compute(5,0,out);
  rules_compute(1,0,expect);
  assert(!memcmp(out,expect,sizeof out));
  puts("GBC bank-3 difficulty/perks: all 192 combinations preserve original rule and mana values PASS");
  return 0;
}
