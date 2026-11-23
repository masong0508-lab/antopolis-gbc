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
  /* The production input loop supplies only player teams 0/1 and
     an explicit Boolean rival modifier; see the real-ROM cursor test. */
  assert(colony_cursor_target(0, 0) == 0);
  assert(colony_cursor_target(1, 1) == 0);
  puts("GBC cursor targets PASS: home/rival symmetry and bounded team IDs");
  return 0;
}
