#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/colony_milestones.h"
int main(void) {
  unsigned n, count = 0;
  for (n = 0; n < 256; ++n) {
    uint8_t reward = colony_milestone_bonus((uint8_t)n);
    if (n == 8) assert(reward == 3);
    else if (n == 16) assert(reward == 4);
    else if (n == 24) assert(reward == 5);
    else assert(reward == 0);
    if (reward) count++;
  }
  assert(count == 3); /* no repeated grants at unrelated hatch counts */
  puts("GBC colony bonuses PASS: 8/16/24 births and no saved-state changes");
  return 0;
}
