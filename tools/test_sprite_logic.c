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
  // Even while periodic walk-pose updates are waiting, switching a sprite
  // slot between carrying and non-carrying must refresh its tile now.
  // Pure OAM type comparison never claims to need another bank switch.
  for (unsigned current = 0; current < 256; current++)
    for (unsigned carry = 0; carry < 2; carry++) {
      unsigned should = (current < 3 || current > 6 ||
                         ((current >= 5) != carry));
      assert(colony_walker_needs_refresh((uint8_t)current,
                                        (uint8_t)carry) == should);
      for (unsigned phase = 0; phase < 8; phase++) {
        uint8_t slot = (uint8_t)((phase * 7u) % 36u);
        carrying[slot] = (uint8_t)carry;
        uint8_t shown = (uint8_t)current;
        if (colony_walker_needs_refresh(shown, (uint8_t)carry))
          shown = colony_walker_frame(slot, (uint8_t)phase, carrying);
        assert(!colony_walker_needs_refresh(shown, (uint8_t)carry));
      }
    }
  // The same animation tiles must retain intuitive team/role identity.
  // Carried gold takes precedence over soldier armor (a fighting ant can
  // also bring back a berry), and both teams use independent palettes.
  for (unsigned team = 0; team < 2; team++) {
    assert(colony_worker_palette((uint8_t)team, 0, 0) == team);
    assert(colony_worker_palette((uint8_t)team, 1, 0) == team + 3);
    assert(colony_worker_palette((uint8_t)team, 0, 1) == team + 5);
    assert(colony_worker_palette((uint8_t)team, 1, 1) == team + 3);
  }
  for (unsigned team = 0; team < 2; team++)
    for (unsigned carry = 0; carry < 2; carry++)
      for (unsigned soldier = 0; soldier < 2; soldier++) {
        unsigned palette = colony_worker_palette((uint8_t)team,
                           (uint8_t)carry, (uint8_t)soldier);
        assert(palette < 7);  // GBC: only seven of eight OBJ palettes used
      }
  puts("GBC worker/carrier/soldier palettes and 36 animated OAM slots PASS");
  return 0;
}
