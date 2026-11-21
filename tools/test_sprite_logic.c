// Native regression for every GBC ant/food-carrying animation slot.
// Shipping bank-3 render code calls this exact pure header helper.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/sprite_logic.h"

int main(void) {
  uint8_t carrying[36], saved[36];
  unsigned carrier, frame, i, seed;
  memset(carrying, 0, sizeof carrying);
  for (i = 0; i < 36; i++) {
    assert(colony_walker_frame((uint8_t)i, 0, carrying) == 3u + (i & 1u));
    assert(colony_walker_frame((uint8_t)i, 1, carrying) == 3u + ((i + 1u) & 1u));
  }
  for (carrier = 0; carrier < 36; carrier++) {
    memset(carrying, 0, sizeof carrying);
    carrying[carrier] = 1;
    memcpy(saved, carrying, sizeof carrying);
    for (frame = 0; frame < 8; frame++) for (i = 0; i < 36; i++) {
      uint8_t want = (uint8_t)((i == carrier ? 5u : 3u)
                              + ((i + frame) & 1u));
      assert(colony_walker_frame((uint8_t)i, (uint8_t)frame, carrying) == want);
    }
    assert(!memcmp(carrying, saved, sizeof carrying));
  }
  // Test all 36 bytes in mixed patterns, including boundaries 7/8 and 31/32.
  for (seed = 0; seed < 256; seed++) {
    for (i = 0; i < 36; i++)
      carrying[i] = (uint8_t)(((seed >> (i & 7u)) ^ (i * 51u)) & 1u);
    for (frame = 0; frame < 4; frame++)
      for (i = 0; i < 36; i++) {
        uint8_t want = (uint8_t)(3u + (carrying[i] ? 2u : 0u)
                                 + ((i + frame) & 1u));
        assert(colony_walker_frame((uint8_t)i, (uint8_t)frame, carrying) == want);
      }
  }
  puts("GBC walking/carrier frames: 36 slots, 256 combinations, no bitshift state PASS");
  return 0;
}
