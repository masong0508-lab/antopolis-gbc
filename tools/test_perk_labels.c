/* Tests the EXACT election-label helper that compiles into ROM bank 3.
   The HUD retains every 7-character name and surrounding label bytes. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/perk_labels.h"
int main(void) {
  static const char names[6][8] = {"FLOOD 4","FASTEGG","MANA UP",
                                   "CALM   ","QUEENUP","SLOWRED"};
  char out[20],again[20];
  for(uint8_t a=0;a<6;a++) for(uint8_t b=0;b<6;b++) {
    memset(out,'?',sizeof out);
    perk_copy_names(out,a,b);
    assert(!memcmp(out+2,names[a],7));
    assert(!memcmp(out+12,names[b],7));
    for(uint8_t i=0;i<20;i++) if((i<2)||(i==9)||(i==10)||(i==11)||(i==19))
      assert(out[i]=='?');
    memcpy(again,out,sizeof out);
    perk_copy_names(again,a,b);
    assert(!memcmp(out,again,sizeof out));
  }
  puts("GBC bank-3 election labels: all 36 selections identical, HUD framing untouched PASS");
  return 0;
}
