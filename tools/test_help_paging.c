/* Bounds-check pause manual one-line and 13-line page jumps. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/help_paging.h"
int main(void) {
  uint32_t total = 0;
  for (uint16_t lines=0;lines<256u;lines++)
    for (uint16_t top=0;top<256u;top++) {
      static const int8_t jumps[]={-13,-1,0,1,13};
      uint8_t max=(uint8_t)(lines>HELP_VISIBLE_LINES?
        lines-HELP_VISIBLE_LINES:0u);
      for (uint8_t i=0;i<sizeof(jumps);i++) {
        int8_t step=jumps[i];
        uint8_t v=ant_help_next_top((uint8_t)top,(uint8_t)lines,step);
        uint8_t t=(uint8_t)(top>max?max:top);
        int16_t intended=(int16_t)t+step;
        if(intended<0)intended=0;
        if(intended>max)intended=max;
        assert(v==(uint8_t)intended);
        assert(v<=max);
        ++total;
      }
    }
  assert(total==327680u);
  assert(ant_help_next_top(0,41,13)==13);
  assert(ant_help_next_top(13,41,13)==26);
  assert(ant_help_next_top(26,41,13)==28);
  assert(ant_help_next_top(28,41,-13)==15);
  assert(ant_help_next_top(0,5,13)==0);
  puts("PASS: help paging clamps all 327680 line/page cases");
  return 0;
}
