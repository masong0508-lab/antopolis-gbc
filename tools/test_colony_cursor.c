#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/colony_cursor.h"
int main(void) {
  uint8_t team, rival, target;
  for (team = 0; team < 2; team++)
    for (rival = 0; rival < 2; rival++) {
      target = colony_cursor_target(team, rival);
      assert(target < 2);
      assert(target == (uint8_t)(rival ? team ^ 1u : team));
    }
  /* A corrupted or unexpected modifier cannot form an out-of-range
     target index: the live game still uses the validated nest tiles. */
  assert(colony_cursor_target(255, 255) == 0);
  assert(colony_cursor_target(254, 255) == 1);
  puts("GBC cursor targets PASS: home/rival symmetry and bounded team IDs");
  return 0;
}
