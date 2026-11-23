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
  if (!heights[y][x] || !heights[(uint8_t)ny][(uint8_t)nx]) return 0;
  change = (int8_t)heights[(uint8_t)ny][(uint8_t)nx] - (int8_t)heights[y][x];
  return (uint8_t)(change >= -1 && change <= 1);
}
static uint8_t eye_try_step(uint8_t heights[EYE_WORLD][EYE_WORLD],
                            uint8_t *x, uint8_t *y,
                            uint8_t facing, int8_t dir) {
  int8_t dx = EYE_DX[facing & 15], dy = EYE_DY[facing & 15];
  // Do not multiply two runtime bytes inside bank 3. SDCC may link the
  // multiplication helper into switchable ROM1, crashing on the first real
  // step even though turning and 3D rendering already work.
  if (dir < 0) { dx = -dx; dy = -dy; }
  // Two orthogonal clearance checks stop sliding through diagonal corners;
  // the destination must obey the same height/cliff rules as ordinary ants.
  if (!eye_legal_leg(heights, *x, *y, dx, dy)) return 0;
  if (dx && dy && (!eye_legal_leg(heights, *x, *y, dx, 0) ||
                   !eye_legal_leg(heights, *x, *y, 0, dy))) return 0;
  *x += dx; *y += dy;
  return 1;
}
// Match the overhead map's four distinct materials in the 3D view.
// Previously the *entire* near grass cell flipped green/sand by (x+y)
// parity. That created conspicuous checkerboard squares and made grass
// look like alternating sandy platforms. Grass is now consistently green;
// the existing tiny per-pixel EYE_GRAIN tufts supply texture instead.
// This is one branch cheaper per visible world sample and needs no assets.
static uint8_t eye_ground_class(uint8_t h, uint8_t x, uint8_t y,
                                uint8_t depth) {
  (void)x; (void)y; (void)depth;
  if (!h) return 4;
  if (h == 1) return 2;
  if (h == 3) return 3;
  return 1;
}
// In the original renderer an ant occupied a broad 128x128 subcell
// and could fill four to six whole screen columns with repeated leg
// patterns. A narrower visual body is still sampled at the cell centre
// but no longer becomes a solid rectangular obstruction. This only
// affects visual size; walking, bite, and occupancy stay unchanged.
static uint8_t eye_entity_centered(uint16_t px, uint16_t py) {
  return (uint8_t)((uint8_t)(px - 88u) < 80u &&
                   (uint8_t)(py - 88u) < 80u);
}
// Body shape is represented by a solid centre and sparse legs at the edge.
 // A narrow 32px subcell forms the core; the remaining visible subcell rim
 // draws only limbs rather than cloning the whole ant into every ray column.
static uint8_t eye_entity_outer(uint16_t px, uint16_t py) {
  return (uint8_t)((uint8_t)(px - 112u) >= 32u ||
                   (uint8_t)(py - 112u) >= 32u);
}
// Ants should be distinct silhouettes, not giant vertical tiled pillars.
// A queen stays larger than a worker; berries remain small in perspective.
static uint8_t eye_entity_height(uint8_t base, uint8_t kind) {
  if (kind == 5) return (uint8_t)(base > 12 ? 12 : (base >> 1));
  if (kind > 2) return (uint8_t)(base > 15 ? 30 : (base << 1));
  return (uint8_t)(base > 21 ? 21 : base);
}
// Gold scent only appears on reachable soil near the camera and near the
// center of a sampled cell. This avoids filling the whole horizon with dots,
// while remaining independent of new sprites/textures/framebuffers.
static uint8_t eye_show_scent(uint8_t pheromone, uint8_t height,
                              uint8_t depth, uint8_t centered) {
  return (uint8_t)(pheromone >= 64 && height >= 1 && height <= 2 &&
                   depth <= 18 && centered);
}
// Banked 3D ray code cannot safely call the compiler's ordinary 16-bit
// multiply helper: in this large GBC build __mulint resides in switchable
// ROM1, which disappears whenever ROM3 is mapped. Multiply by a signed
// small column offset using shifts and additions in the SAME bank.
static int16_t eye_mul_small(int8_t sine, int8_t column) {
  int16_t addend = sine, sum = 0;
  uint8_t magnitude = (uint8_t)(column < 0 ? -(int16_t)column : column);
  while (magnitude) {
    if (magnitude & 1u) sum += addend;
    addend += addend;
    magnitude >>= 1;
  }
  return column < 0 ? -sum : sum;
}
// ATTACK and WALK share a single facing/corner rule. A bite checks the tile
// ahead WITHOUT moving the scout. Avoid phasing through walls or water and
// avoid 4-way attacks aimed away from the visual 3D facing.
static uint8_t eye_attack_target(uint8_t heights[EYE_WORLD][EYE_WORLD],
                                 uint8_t x, uint8_t y, uint8_t facing,
                                 uint8_t *tx, uint8_t *ty) {
  if (!eye_try_step(heights, &x, &y, facing, 1)) return 0;
  *tx = x; *ty = y;
  return 1;
}
// Status code for the cell directly ahead, sharing the same real collision
// rule as ANT EYE walking and biting. No map allocations or extra camera rays.
// 0 boundary, 1 water, 2 blocked cliff/corner, 3 empty, 4 food,
// 5 friendly worker, 6 enemy worker, 7 home queen, 8 enemy queen.
static uint8_t eye_ahead_kind(uint8_t heights[EYE_WORLD][EYE_WORLD],
                              uint8_t entities[EYE_WORLD][EYE_WORLD],
                              uint8_t x, uint8_t y, uint8_t facing) {
  int8_t nx = (int8_t)x + EYE_DX[facing & 15];
  int8_t ny = (int8_t)y + EYE_DY[facing & 15];
  uint8_t tx, ty, o;
  if (nx < 0 || ny < 0 || nx >= EYE_WORLD || ny >= EYE_WORLD) return 0;
  if (!heights[(uint8_t)ny][(uint8_t)nx]) return 1;
  if (!eye_attack_target(heights, x, y, facing, &tx, &ty)) return 2;
  o = entities[ty][tx];
  if (!o) return 3;
  if (o == 5) return 4;
  if (o == 1) return 5;
  if (o == 2) return 6;
  if (o == 3) return 7;
  return 8;
}
#ifndef EYE_NAV_RADIUS
#define EYE_NAV_RADIUS 8
#endif
// The food bearing must describe an ACTUALLY WALKABLE first step. Reject
// diagonal corner cuts and cliffs exactly as ANT EYE movement does. If a
// diagonal is obstructed but one cardinal leg is open, guide along that
// side instead of pointing straight into a wall. No heap/queue/map RAM.
/* Avoid a two-tile dead end: after the first cardinal step there must
 * be a second legal step toward the berry, not a turn straight back.
 * Called only by the bounded food scan on worker movement, never a ray. */
static uint8_t eye_food_followup(uint8_t heights[EYE_WORLD][EYE_WORLD],
                                 uint8_t x, uint8_t y,
                                 uint8_t tx, uint8_t ty,
                                 int8_t sx, int8_t sy) {
  uint8_t nx = (uint8_t)((int8_t)x + sx);
  uint8_t ny = (uint8_t)((int8_t)y + sy);
  int8_t dx = tx > nx ? 1 : tx < nx ? -1 : 0;
  int8_t dy = ty > ny ? 1 : ty < ny ? -1 : 0;
  if (nx == tx && ny == ty) return 1;
  return (uint8_t)((dx && dx != -sx &&
                    eye_legal_leg(heights, nx, ny, dx, 0)) ||
                   (dy && dy != -sy &&
                    eye_legal_leg(heights, nx, ny, 0, dy)));
}
/* FOOD bearings are actual safe next moves, with one extra lookahead.
 * When direct progress is blocked along a single axis, point to a
 * two-step sidestep instead. It is LOCAL guidance, not claimed BFS. */
static uint8_t eye_food_first_leg(uint8_t heights[EYE_WORLD][EYE_WORLD],
                                  uint8_t vx, uint8_t vy,
                                  uint8_t tx, uint8_t ty) {
  int8_t dx = tx > vx ? 1 : tx < vx ? -1 : 0;
  int8_t dy = ty > vy ? 1 : ty < vy ? -1 : 0;
  uint8_t horiz = dx > 0 ? 1u : dx < 0 ? 2u : 0u;
  uint8_t vert = dy > 0 ? 4u : dy < 0 ? 8u : 0u;
  // A legal diagonal landing can still enter a one-step pocket with
  // no forward exit toward the berry. Require the same cheap second
  // progress step as cardinal food guidance before advertising it.
  if (dx && dy &&
      eye_legal_leg(heights, vx, vy, dx, dy) &&
      eye_legal_leg(heights, vx, vy, dx, 0) &&
      eye_legal_leg(heights, vx, vy, 0, dy) &&
      eye_food_followup(heights, vx, vy, tx, ty, dx, dy))
    return (uint8_t)(horiz | vert);
  if (dx && eye_legal_leg(heights, vx, vy, dx, 0) &&
      eye_food_followup(heights, vx, vy, tx, ty, dx, 0)) return horiz;
  if (dy && eye_legal_leg(heights, vx, vy, 0, dy) &&
      eye_food_followup(heights, vx, vy, tx, ty, 0, dy)) return vert;
  /* Only use the shallow detour for axis-aligned goals. A speculative
     diagonal detour could prefer an unreachable berry over reachable
     food elsewhere, and could oscillate at blocked corners. */
  if (!dx && dy) {
    if (eye_legal_leg(heights, vx, vy, 1, 0) &&
        eye_legal_leg(heights, (uint8_t)(vx + 1u), vy, 0, dy)) return 1u;
    if (eye_legal_leg(heights, vx, vy, -1, 0) &&
        eye_legal_leg(heights, (uint8_t)(vx - 1u), vy, 0, dy)) return 2u;
  } else if (!dy && dx) {
    if (eye_legal_leg(heights, vx, vy, 0, -1) &&
        eye_legal_leg(heights, vx, (uint8_t)(vy - 1u), dx, 0)) return 8u;
    if (eye_legal_leg(heights, vx, vy, 0, 1) &&
        eye_legal_leg(heights, vx, (uint8_t)(vy + 1u), dx, 0)) return 4u;
  }
  return 0;
}
// Food scan is bounded to a configurable 8-tile radius and only runs when the selected
// ant CHANGES TILE, not on every ray/turn/frame. Return x/y bearing bits.
static uint8_t eye_food_bearing(uint8_t heights[EYE_WORLD][EYE_WORLD],
                                uint8_t berries[EYE_WORLD][EYE_WORLD],
                                uint8_t vx, uint8_t vy) {
  uint8_t x, y, best = 255, d, leg, bearing = 0;
  uint8_t x0 = vx > EYE_NAV_RADIUS ? vx - EYE_NAV_RADIUS : 0;
  uint8_t x1 = vx < (31 - EYE_NAV_RADIUS) ? vx + EYE_NAV_RADIUS : 31;
  uint8_t y0 = vy > EYE_NAV_RADIUS ? vy - EYE_NAV_RADIUS : 0;
  uint8_t y1 = vy < (31 - EYE_NAV_RADIUS) ? vy + EYE_NAV_RADIUS : 31;
  for (y = y0; y <= y1; y++) for (x = x0; x <= x1; x++) {
    if (!berries[y][x] || !heights[y][x]) continue;
    d = (uint8_t)((x > vx ? x - vx : vx - x) +
                  (y > vy ? y - vy : vy - y));
    // No first step is necessary when a berry is under this worker.
    // This also avoids finishing the radius scan once food is found here.
    if (d == 0u) return 0x90u; // FOOD: HERE, NEAR
    if (d < best) {
      leg = eye_food_first_leg(heights, vx, vy, x, y);
      if (leg) { best = d; bearing = leg; }
    }
  }
  /* The direction field uses bits 0..3. Spare bits 4/5 encode
     coarse proximity without another world scan, ray cast or RAM map.
     They are Manhattan DISTANCE bands, not promised travel times:
     NEAR <=4, MID 5..8, FAR >=9 squares. */
  return best == 255 ? 0 : (uint8_t)(0x80u | bearing |
            (best <= 4u ? 0x10u : best >= 9u ? 0x20u : 0u));
}
#endif
