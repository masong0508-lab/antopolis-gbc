// Small pure ANT EYE movement/foraging rules. This header is compiled into
// switchable ROM bank 3 and tested natively; no framebuffer or extra world map.
#ifndef ANTOPOLIS_EYE_NAVIGATION_H
#define ANTOPOLIS_EYE_NAVIGATION_H
#include <stdint.h>
#define EYE_WORLD 32
// Sixteen facing angles quantized to eight *movement* directions. Camera
// still turns in 22.5 degree increments; walking now follows diagonals.
static const int8_t EYE_DX[16] = { 1,1,1,1,0,-1,-1,-1,-1,-1,-1,-1,0,1,1,1 };
static const int8_t EYE_DY[16] = { 0,0,1,1,1,1,1,0,0,0,-1,-1,-1,-1,-1,0 };
static uint8_t eye_legal_leg(uint8_t heights[EYE_WORLD][EYE_WORLD],
                             uint8_t x, uint8_t y, int8_t dx, int8_t dy) {
  int8_t nx = (int8_t)x + dx, ny = (int8_t)y + dy, change;
  if (nx < 0 || ny < 0 || nx >= EYE_WORLD || ny >= EYE_WORLD) return 0;
  if (!heights[(uint8_t)ny][(uint8_t)nx]) return 0;
  change = (int8_t)heights[(uint8_t)ny][(uint8_t)nx] - (int8_t)heights[y][x];
  return (uint8_t)(change >= -1 && change <= 1);
}
static uint8_t eye_try_step(uint8_t heights[EYE_WORLD][EYE_WORLD],
                            uint8_t *x, uint8_t *y,
                            uint8_t facing, int8_t dir) {
  int8_t dx = EYE_DX[facing & 15], dy = EYE_DY[facing & 15];
  dx *= dir; dy *= dir;
  // Two orthogonal clearance checks stop sliding through diagonal corners;
  // the destination must obey the same height/cliff rules as ordinary ants.
  if (!eye_legal_leg(heights, *x, *y, dx, dy)) return 0;
  if (dx && dy && (!eye_legal_leg(heights, *x, *y, dx, 0) ||
                   !eye_legal_leg(heights, *x, *y, 0, dy))) return 0;
  *x += dx; *y += dy;
  return 1;
}
// Color IDs consumed by ANT EYE's existing palette mapper. Distant grass
// shares one darker shade rather than the close-up checker pattern; height-3
// walls remain brown and water stays blue through the water palette.
static uint8_t eye_ground_class(uint8_t h, uint8_t x, uint8_t y,
                                uint8_t depth) {
  if (!h) return 4;
  if (h == 3) return 3;
  return (uint8_t)(depth > 14 || !((x + y) & 1u) ? 2u : 1u);
}
// Food scan is bounded to an 8-tile radius and only runs when the selected
// ant CHANGES TILE, not on every ray/turn/frame. Return x/y bearing bits.
static uint8_t eye_food_bearing(uint8_t heights[EYE_WORLD][EYE_WORLD],
                                uint8_t berries[EYE_WORLD][EYE_WORLD],
                                uint8_t vx, uint8_t vy) {
  uint8_t x, y, best = 255, tx = vx, ty = vy, d;
  uint8_t x0 = vx > 8 ? vx - 8 : 0;
  uint8_t x1 = vx < 23 ? vx + 8 : 31;
  uint8_t y0 = vy > 8 ? vy - 8 : 0;
  uint8_t y1 = vy < 23 ? vy + 8 : 31;
  for (y = y0; y <= y1; y++) for (x = x0; x <= x1; x++) {
    if (!berries[y][x] || !heights[y][x]) continue;
    d = (uint8_t)((x > vx ? x - vx : vx - x) +
                  (y > vy ? y - vy : vy - y));
    if (d < best) { best = d; tx = x; ty = y; }
  }
  return best == 255 ? 0 : (uint8_t)(0x80u |
    (tx > vx ? 1u : tx < vx ? 2u : 0u) |
    (ty > vy ? 4u : ty < vy ? 8u : 0u));
}
#endif
