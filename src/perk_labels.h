/* The six seven-column perk names are only needed during elections.
   Keep these 42 ROM bytes with the bank-3 UI builder, not in the
   always-mapped and nearly full fixed game-code bank. */
#ifndef ANTOPOLIS_PERK_LABELS_H
#define ANTOPOLIS_PERK_LABELS_H
#include <stdint.h>
static void perk_copy_names(char *out, uint8_t first, uint8_t second) {
  static const char NAMES[] = "FLOOD 4FASTEGGMANA UPCALM   QUEENUPSLOWRED";
  uint8_t i, a=(uint8_t)((first<<3)-first);
  uint8_t b=(uint8_t)((second<<3)-second);
  for(i=0;i<7u;i++) {
    out[2u+i]=NAMES[a+i];
    out[12u+i]=NAMES[b+i];
  }
}
#endif
