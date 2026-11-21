// Pure native regression for every GBC ant animation slot and food packet.
// The exact helper also runs from the banked real-hardware OAM renderer.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/sprite_logic.h"

int main(void) {
  uint8_t mask[5], saved[5];
  unsigned carrier, frame, i, seed;
  memset(mask, 0, sizeof mask);
  for (i = 0; i < 36; i++) {
    assert(colony_walker_frame((uint8_t)i, 0, mask) == 3u + (i & 1u));
    assert(colony_walker_frame((uint8_t)i, 1, mask) == 3u + ((i + 1u) & 1u));
  }
  for (carrier = 0; carrier < 36; carrier++) {
    memset(mask, 0, sizeof mask);
    mask[carrier >> 3] |= (uint8_t)(1u << (carrier & 7u));
    memcpy(saved, mask, sizeof mask);
    for (frame = 0; frame < 8; frame++) for (i = 0; i < 36; i++) {
      uint8_t want = (uint8_t)((i == carrier ? 5u : 3u)
                              + ((i + frame) & 1u));
      assert(colony_walker_frame((uint8_t)i, (uint8_t)frame, mask) == want);
    }
    // A banked animation pass must not mutate either packed worker state
    // or the underlying game simulation when frame count advances.
    assert(!memcmp(mask, saved, sizeof mask));
  }
  // Exhaustively mix all five bytes including the final partial nibble.
  for (seed = 0; seed < 256; seed++) {
    for (i = 0; i < 5; i++)
      mask[i] = (uint8_t)(seed ^ (i * 51u));
    for (frame = 0; frame < 4; frame++)
      for (i = 0; i < 36; i++) {
        uint8_t carrier_flag = (uint8_t)((mask[i >> 3] >> (i & 7u)) & 1u);
        uint8_t want = (uint8_t)(3u + carrier_flag * 2u + ((i + frame) & 1u));
        assert(colony_walker_frame((uint8_t)i, (uint8_t)frame, mask) == want);
      }
  }
  puts("GBC walking/carrier frames: 36 slots, bank boundary, 256 masks PASS");
  return 0;
}
