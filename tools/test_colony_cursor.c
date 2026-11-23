#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/colony_cursor.h"
int main(void) {
  uint8_t nests_x[2] = {3, 29}, nests_y[2] = {9, 22}, x = 11, y = 14;
  assert(!colony_cursor_warp(0, 0, nests_x, nests_y, &x, &y));
  assert(x == 11 && y == 14);
  assert(colony_cursor_warp(1, 0, nests_x, nests_y, &x, &y));
  assert(x == 3 && y == 9);
  assert(colony_cursor_warp(2, 0, nests_x, nests_y, &x, &y));
  assert(x == 29 && y == 22);
  assert(colony_cursor_warp(1, 1, nests_x, nests_y, &x, &y));
  assert(x == 29 && y == 22);
  assert(colony_cursor_warp(2, 1, nests_x, nests_y, &x, &y));
  assert(x == 3 && y == 9);
  assert(!colony_cursor_warp(3, 0, nests_x, nests_y, &x, &y));
  assert(!colony_cursor_warp(1, 2, nests_x, nests_y, &x, &y));
  assert(x == 3 && y == 9);
  nests_x[1] = 32; nests_y[1] = 33;
  assert(!colony_cursor_warp(2, 0, nests_x, nests_y, &x, &y));
  assert(x == 3 && y == 9);
  nests_x[1] = 31; nests_y[1] = 0;
  assert(colony_cursor_warp(2, 0, nests_x, nests_y, &x, &y));
  assert(x == 31 && y == 0);
  puts("GBC cursor jump PASS: own/rival, bounds, both players, no unintended writes");
  return 0;
}
