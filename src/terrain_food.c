/* Normalize food after importing a GBC battery save. This scan occurs
   only when loading a match, never during gameplay or ANT EYE ray frames.
   The caller must guard the bank-1 music interrupt while ROM bank 3 is
   selected. No extra array, heap allocation or SRAM format change. */
#ifndef TERRAIN_FOOD_HOST_TEST
#pragma bank 3
#include <gb/gb.h>
#else
#define BANKED
#endif
#include <stdint.h>
void terrain_food_normalize(uint8_t heights[32][32],
                            uint8_t berries[32][32]) BANKED {
  uint8_t x,y;
  for (y=0; y<32u; ++y)
    for (x=0; x<32u; ++x)
      if (!heights[y][x]) berries[y][x]=0u;
}
