// Host regressions for bank-3 GBC first-person rules. No GBDK needed.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/eye_navigation.h"
static uint8_t terrain[32][32], berries[32][32];
int main(void) {
  uint8_t x, y, bearing, i;
  memset(terrain, 1, sizeof terrain);
  for (i = 0; i < 16; i++) {
    x = 10; y = 10;
    assert(eye_try_step(terrain, &x, &y, i, 1));
    assert(x == (uint8_t)(10 + EYE_DX[i]));
    assert(y == (uint8_t)(10 + EYE_DY[i]));
    assert(eye_try_step(terrain, &x, &y, i, -1));
    assert(x == 10 && y == 10);
  }
  // Cannot walk off map even when the camera faces the boundary.
  x = 0; y = 0;
  assert(!eye_try_step(terrain, &x, &y, 8, 1));
  assert(x == 0 && y == 0);
  // Prevent diagonal corner cutting through water and cliffs.
  x = 10; y = 10;
  terrain[10][11] = 0;
  assert(!eye_try_step(terrain, &x, &y, 2, 1));
  terrain[10][11] = 1;
  terrain[11][11] = 3;
  assert(!eye_try_step(terrain, &x, &y, 2, 1));
  terrain[11][11] = 1;
  assert(eye_try_step(terrain, &x, &y, 2, 1));
  assert(x == 11 && y == 11);
  // Nearby food can be found; directions are canonical compass axes.
  assert(!eye_food_bearing(terrain, berries, 10, 10));
  berries[10][13] = 1;
  berries[8][9] = 1;
  bearing = eye_food_bearing(terrain, berries, 10, 10);
  assert(bearing == (uint8_t)(0x80 | 2 | 8)); // west, north
  // Farther than eight tiles is not shown as a close source.
  berries[8][9] = berries[10][13] = 0;
  berries[30][30] = 1;
  assert(!eye_food_bearing(terrain, berries, 10, 10));
  // Terrain classes are stable across near/distant columns, with a distinct
  // water class, brown high land, and darkened grass beyond fourteen steps.
  assert(eye_ground_class(0, 0, 0, 1) == 4);
  assert(eye_ground_class(3, 0, 0, 1) == 3);
  assert(eye_ground_class(1, 2, 3, 2) == 1);
  assert(eye_ground_class(2, 2, 3, 15) == 2);
  assert(eye_ground_class(1, 2, 2, 2) == 2);
  puts("GBC ANT EYE movement, collisions, food guidance and terrain shading passed");
  return 0;
}
