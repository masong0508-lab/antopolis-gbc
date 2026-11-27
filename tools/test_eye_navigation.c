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
  // A still-alive worker waiting for its AI drowning slice cannot use
  // paused first-person movement or biting to escape its submerged cell.
  x = 10; y = 10; terrain[10][10] = 0;
  assert(!eye_try_step(terrain, &x, &y, 0, 1));
  assert(x == 10 && y == 10);
  assert(!eye_try_step(terrain, &x, &y, 0, -1));
  assert(!eye_attack_target(terrain, x, y, 0, &x, &y));
  assert(x == 10 && y == 10);
  terrain[10][10] = 1;
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
  // Attack uses exactly the same direction as a forward step but never
  // changes the attacking ant's position. Blocked diagonal corners, water,
  // cliff jumps and edges must never allow a bite through geometry.
  x = 10; y = 10;
  assert(eye_attack_target(terrain, x, y, 2, &x, &y));
  assert(x == 11 && y == 11);
  x = 10; y = 10; terrain[10][11] = 0;
  assert(!eye_attack_target(terrain, x, y, 2, &x, &y));
  assert(x == 10 && y == 10);
  terrain[10][11] = 1;
  assert(eye_attack_target(terrain, x, y, 0, &x, &y));
  assert(x == 11 && y == 10);
  x = 0; y = 0;
  assert(!eye_attack_target(terrain, x, y, 8, &x, &y));
  // The ANT EYE target HUD must agree with the real movement and bite
  // collision rules and identify real objects, not imaginary 3D markers.
  memset(berries, 0, sizeof berries);
  assert(eye_ahead_kind(terrain, berries, 0, 0, 8) == 0); // edge
  terrain[10][11] = 0;
  assert(eye_ahead_kind(terrain, berries, 10, 10, 0) == 1); // water
  terrain[10][11] = 3;
  assert(eye_ahead_kind(terrain, berries, 10, 10, 0) == 2); // high step
  terrain[10][11] = 1;
  assert(eye_ahead_kind(terrain, berries, 10, 10, 0) == 3); // free
  for (i = 1; i <= 5; i++) {
    berries[10][11] = i;
    assert(eye_ahead_kind(terrain, berries, 10, 10, 0) ==
           (uint8_t)(i == 5 ? 4 : i + 4));
  }
  berries[10][11] = 0;
  terrain[10][11] = 0;
  assert(eye_ahead_kind(terrain, berries, 10, 10, 2) == 2); // diagonal water corner
  terrain[10][11] = 1;
  // Nearby food can be found; directions are canonical compass axes.
  assert(!eye_food_bearing(terrain, berries, 10, 10));
  // Food underfoot is immediately reachable even though walking needs
  // no first leg. Nearby earlier-scanned fruit must not hide FOOD: HERE.
  berries[10][10] = 1;
  assert(eye_food_bearing(terrain, berries, 10, 10) == (uint8_t)(0x80 | 0x10));
  berries[9][10] = 1;
  assert(eye_food_bearing(terrain, berries, 10, 10) == (uint8_t)(0x80 | 0x10));
  berries[9][10] = berries[10][10] = 0;
  // The early underfoot check must not report submerged food as reachable.
  berries[10][10] = 1; terrain[10][10] = 0;
  assert(!eye_food_bearing(terrain, berries, 10, 10));
  terrain[10][10] = 1; berries[10][10] = 0;
  // Both corners use the same early underfoot result.
  berries[0][0] = berries[31][31] = 1;
  assert(eye_food_bearing(terrain, berries, 0, 0) == 0x90u);
  assert(eye_food_bearing(terrain, berries, 31, 31) == 0x90u);
  berries[0][0] = berries[31][31] = 0;
  berries[10][13] = 1;
  berries[8][9] = 1;
  bearing = eye_food_bearing(terrain, berries, 10, 10);
  assert(bearing == (uint8_t)(0x80 | 0x10 | 2 | 8)); // west, north
  // An obstructed diagonal must never be advertised as a valid step.
  terrain[9][10] = 0;   // north water, west still clear
  assert(eye_food_bearing(terrain, berries, 10, 10) ==
         (uint8_t)(0x80 | 0x10 | 2)); // steer west, not NW
  terrain[10][9] = 0;   // nearest NW food now unreachable at first step
  assert(eye_food_bearing(terrain, berries, 10, 10) ==
         (uint8_t)(0x80 | 0x10 | 1)); // use farther reachable eastern food
  berries[10][13] = 0;
  assert(eye_food_bearing(terrain, berries, 10, 10) == 0x40u);
  terrain[9][10] = 1;
  terrain[10][9] = 1;
  // A free diagonal landing is not enough: if both onward legs toward
  // the berry are water, show a safe cardinal approach instead.
  memset(berries, 0, sizeof berries);
  berries[12][12] = 1;
  terrain[11][12] = terrain[12][11] = 0;
  assert(eye_food_first_leg(terrain, 10, 10, 12, 12) == 1);
  assert(eye_food_bearing(terrain, berries, 10, 10) ==
         (uint8_t)(0x80 | 0x10 | 1));
  terrain[11][12] = terrain[12][11] = 1;
  assert(eye_food_first_leg(terrain, 10, 10, 12, 12) == (1 | 4));
  berries[12][12] = 0;
  // Farther than eight tiles is not shown as a close source.
  berries[8][9] = berries[10][13] = 0;
  berries[30][30] = 1;
  assert(!eye_food_bearing(terrain, berries, 10, 10));
  // A northward berry behind one blocked step needs a safe east
  // sidestep. From that sidestep, the HUD must NOT steer west again.
  memset(berries, 0, sizeof berries);
  berries[8][10] = 1;
  terrain[9][10] = 0;
  assert(eye_food_bearing(terrain, berries, 10, 10) == (uint8_t)(0x80 | 0x10 | 1));
  assert(eye_food_bearing(terrain, berries, 11, 10) == (uint8_t)(0x80 | 0x10 | 8));
  assert(eye_food_bearing(terrain, berries, 11, 9) == (uint8_t)(0x80 | 0x10 | 8));
  assert(eye_food_bearing(terrain, berries, 11, 8) == (uint8_t)(0x80 | 0x10 | 2));
  // Neither bypass has a legal second step: no invented bearing.
  terrain[9][11] = terrain[9][9] = 0;
  assert(eye_food_bearing(terrain, berries, 10, 10) == 0x40u);
  terrain[9][10] = terrain[9][11] = terrain[9][9] = 1;
  memset(berries, 0, sizeof berries);
  // A one-step berry is globally optimal. Preserve original row-order
  // tie breaking, including blocked neighboring berries and map edges.
  memset(berries, 0, sizeof berries);
  berries[9][10] = berries[10][9] = berries[10][11] = berries[11][10] = 1;
  assert(eye_food_bearing(terrain, berries, 10, 10) == 0x98u);
  terrain[9][10] = 0;
  assert(eye_food_bearing(terrain, berries, 10, 10) == 0x92u);
  terrain[10][9] = 3;
  // West needs a detour; the directly accessible east berry wins.
  assert(eye_food_bearing(terrain, berries, 10, 10) == 0x91u);
  // With no directly reachable berry, still offer the original detour.
  berries[10][11] = berries[11][10] = 0;
  assert(eye_food_bearing(terrain, berries, 10, 10) == 0x94u);
  berries[10][11] = berries[11][10] = 1;
  terrain[11][9] = 0;
  assert(eye_food_bearing(terrain, berries, 10, 10) == 0x91u);
  terrain[10][11] = 0;
  assert(eye_food_bearing(terrain, berries, 10, 10) == 0x94u);
  terrain[9][10] = terrain[10][9] = terrain[10][11] = terrain[11][9] = 1;
  memset(berries, 0, sizeof berries);
  berries[0][1] = berries[1][0] = 1;
  assert(eye_food_bearing(terrain, berries, 0, 0) == 0x91u);
  memset(berries, 0, sizeof berries);
  berries[30][31] = berries[31][30] = 1;
  assert(eye_food_bearing(terrain, berries, 31, 31) == 0x98u);
  memset(berries, 0, sizeof berries);
  // Diagonal pickups are also a single legal move. Prefer one to
  // a closer-looking cardinal berry that requires a cliff detour.
  memset(berries, 0, sizeof berries);
  berries[10][9] = berries[11][11] = 1;
  terrain[10][9] = 3;
  assert(eye_food_bearing(terrain, berries, 10, 10) == 0x95u);
  // An actual cardinal pickup retains priority.
  berries[10][11] = 1;
  assert(eye_food_bearing(terrain, berries, 10, 10) == 0x91u);
  berries[10][11] = 0;
  terrain[10][11] = 0;
  // East blocks the diagonal. The old west-cliff detour prefers north.
  assert(eye_food_bearing(terrain, berries, 10, 10) == 0x98u);
  terrain[10][9] = terrain[10][11] = 1;
  memset(berries, 0, sizeof berries);
  berries[9][9] = berries[9][11] = berries[11][9] = berries[11][11] = 1;
  assert(eye_food_bearing(terrain, berries, 10, 10) == 0x9Au);
  terrain[9][10] = 0;
  assert(eye_food_bearing(terrain, berries, 10, 10) == 0x96u);
  terrain[9][10] = 1;
  memset(berries, 0, sizeof berries);
  berries[1][1] = 1; berries[30][30] = 1;
  assert(eye_food_bearing(terrain, berries, 0, 0) == 0x95u);
  assert(eye_food_bearing(terrain, berries, 31, 31) == 0x9Au);
  memset(berries, 0, sizeof berries);
  // Carriers preserve their team identity in the forward target HUD.
  memset(terrain,1,sizeof terrain); memset(berries,0,sizeof berries);
  berries[10][11]=9;
  assert(eye_ahead_kind(terrain, berries, 10, 10, 0)==9);
  berries[10][11]=10;
  assert(eye_ahead_kind(terrain, berries, 10, 10, 0)==10);
  terrain[10][11]=3;
  assert(eye_ahead_kind(terrain, berries, 10, 10, 0)==2);
  terrain[10][11]=1; berries[10][11]=0;
  // Proximity is encoded in otherwise unused compass bits. These are
  // approximate map distances, NOT a count of guaranteed walking steps.
  berries[10][14] = 1; // distance 4: NEAR
  assert(eye_food_bearing(terrain, berries, 10, 10) ==
         (uint8_t)(0x80 | 0x10 | 1));
  berries[10][14] = 0; berries[10][15] = 1; // 5: MID
  assert(eye_food_bearing(terrain, berries, 10, 10) ==
         (uint8_t)(0x80 | 1));
  berries[10][15] = 0; berries[10][18] = 1; // 8: MID
  assert(eye_food_bearing(terrain, berries, 10, 10) ==
         (uint8_t)(0x80 | 1));
  berries[10][18] = 0; berries[11][18] = 1; // 9: FAR
  assert(eye_food_bearing(terrain, berries, 10, 10) ==
         (uint8_t)(0x80 | 0x20 | 1 | 4));
  berries[11][18] = 0;
  // Reproduce every ANT EYE ray column term, including negative sines,
  // without crossing banks to the 16-bit SDCC multiply helper.
  for (int16_t s = -64; s <= 64; s += 8)
    for (int16_t c = -19; c <= 19; c += 2)
      assert(eye_mul_small((int8_t)s, (int8_t)c) == s * c);
  // Creature visuals should remain compact and centred on actual world
  // cells; unlike the old 128px-wide hitbox, grazing rays cannot paint
  // a multi-column checker wall. These checks do not change combat rules.
  assert(eye_entity_centered(128, 128));
  assert(eye_entity_centered(88, 88));
  assert(!eye_entity_centered(87, 128));
  assert(!eye_entity_centered(168, 128));
  assert(!eye_entity_centered(128, 87));
  assert(!eye_entity_centered(128, 168));
  assert(eye_entity_centered(384, 640)); // world subcell, not absolute tile
  // A whole body must not be duplicated in neighbouring ray columns.
  assert(!eye_entity_outer(128, 128));
  assert(!eye_entity_outer(112, 112));
  assert(!eye_entity_outer(143, 143));
  assert(eye_entity_outer(111, 128));
  assert(eye_entity_outer(144, 128));
  assert(eye_entity_outer(128, 111));
  assert(eye_entity_outer(128, 144));
  assert(eye_entity_height(40, 1) == 21);
  assert(eye_entity_height(15, 1) == 15);
  assert(eye_entity_height(40, 3) == 30);
  assert(eye_entity_height(10, 3) == 20);
  assert(eye_entity_height(40, 5) == 12);
  assert(eye_entity_height(10, 5) == 5);
  // Pheromone is visible only on nearby soil in the center of the world
  // cell. Water and high cliffs cannot generate false gold trail markers.
  assert(eye_show_scent(255, 1, 4, 1));
  assert(eye_show_scent(64, 2, 18, 1));
  assert(!eye_show_scent(63, 1, 4, 1));
  assert(!eye_show_scent(255, 0, 4, 1));
  assert(!eye_show_scent(255, 3, 4, 1));
  assert(!eye_show_scent(255, 1, 19, 1));
  assert(!eye_show_scent(255, 1, 4, 0));
  // World terrain must have stable material colors across x/y and
  // distance. No full grass-cell checkerboard: per-pixel renderer grain
  // already provides the natural surface texture at much finer scale.
  assert(eye_ground_class(0, 0, 0, 1) == 4);
  assert(eye_ground_class(1, 2, 3, 2) == 2);
  assert(eye_ground_class(1, 2, 2, 22) == 2);
  for (y = 0; y < 32; y++)
    for (x = 0; x < 32; x++) {
      assert(eye_ground_class(2, x, y, 1) == 1);
      assert(eye_ground_class(2, x, y, 24) == 1);
    }
  assert(eye_ground_class(3, 0, 0, 1) == 3);
  puts("GBC ANT EYE movement, targeting, food, scent visibility and terrain shading passed");
  return 0;
}
