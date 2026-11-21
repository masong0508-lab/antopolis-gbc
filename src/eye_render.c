// First-person ray marcher, kept in switchable ROM bank 3 to preserve the scarce fixed bank.
// Only the pure per-column ray marcher is banked; VRAM writes remain in the main bank.
#pragma bank 3
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include "eye_navigation.h"
#include "sprite_logic.h"
#include "eye_surface_detail.h"
#define W 32
#define H 32
#define VX 20
#define VR 12
#define VY0 2
#define VN 24
extern uint8_t hgt[H][W], food[H][W], ph[H][W], occ[H][W], cls[VR * 8], vang;
extern int8_t OFF[7][VN + 1];
static const int8_t SIN16[16] = {0, 24, 45, 59, 64, 59, 45, 24, 0, -24, -45, -59, -64, -59, -45, -24};
static const uint8_t BBH[VN + 1] = {0, 40, 32, 21, 16, 12, 10, 9, 8, 7, 6, 5, 5, 4, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2};

// Frozen perspective offsets derived from the original formula:
// clamp((((height - eye_height) * 4 - 3) * 16) / depth, -64, 64).
// Keeping the table in bank 3 saves fixed ROM and avoids any __divsint call
// while a switchable bank is mapped. Index 0 is unused and stays zero.
static const int8_t EYE_OFFSETS[7][25] = {
  {0,-64,-64,-64,-60,-48,-40,-34,-30,-26,-24,-21,-20,-18,-17,-16,-15,-14,-13,-12,-12,-11,-10,-10,-10},
  {0,-64,-64,-58,-44,-35,-29,-25,-22,-19,-17,-16,-14,-13,-12,-11,-11,-10,-9,-9,-8,-8,-8,-7,-7},
  {0,-64,-56,-37,-28,-22,-18,-16,-14,-12,-11,-10,-9,-8,-8,-7,-7,-6,-6,-5,-5,-5,-5,-4,-4},
  {0,-48,-24,-16,-12,-9,-8,-6,-6,-5,-4,-4,-4,-3,-3,-3,-3,-2,-2,-2,-2,-2,-2,-2,-2},
  {0,16,8,5,4,3,2,2,2,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},
  {0,64,40,26,20,16,13,11,10,8,8,7,6,6,5,5,5,4,4,4,4,3,3,3,3},
  {0,64,64,48,36,28,24,20,18,16,14,13,12,11,10,9,9,8,8,7,7,6,6,6,6}
};
void eye_init_offsets(void) BANKED {
  int8_t *destination = &OFF[0][0];
  const int8_t *source = &EYE_OFFSETS[0][0];
  uint8_t i;
  for (i = 0; i < 175; ++i) *destination++ = *source++;
}


void view_col(uint8_t c, uint8_t vx, uint8_t vy, uint8_t eh) BANKED {
  int16_t t = (int16_t)c * 2 - 19, px = (int16_t)vx * 256 + 128, py = (int16_t)vy * 256 + 128, yt, a, e, r, rx, ry;
  uint8_t n, x, y, h, o, col, rim, lim = VR * 8, lb;
  // Never use 16-bit variable '*' inside a banked routine. GBDK would
  // call __mulint from switchable ROM1, switching out this ROM3 routine.
  rx = (int16_t)SIN16[(vang + 4) & 15] * 32 -
       eye_mul_small(SIN16[vang], (int8_t)t);
  ry = (int16_t)SIN16[vang] * 32 +
       eye_mul_small(SIN16[(vang + 4) & 15], (int8_t)t);
  rx >>= 4; ry >>= 4;                                // one step = half a cell forward
  for (n = 0; n < VR * 8; n++) cls[n] = 0;
  for (n = 1; n <= VN && lim; n++) {
    px += rx; py += ry;
    if (px < 0 || py < 0 || px >= W * 256 || py >= H * 256) break;
    x = (uint8_t)(px >> 8); y = (uint8_t)(py >> 8);
    h = hgt[y][x];
    yt = 48 - OFF[h + 3 - eh][n];
    lb = lim;
    if (yt < lb) {                                   // terrain pokes above everything nearer: paint it
      a = yt < 0 ? 0 : yt;
      col = eye_ground_class(h, x, y, n);
      for (r = a; r < lb; r++) cls[r] = col;
      // Physical depth: foam only next to REAL land, laminated rock
      // edges, and occasional near-field grass tufts or sand pebbles.
      // These color classes reuse existing CGB palettes/96px column RAM.
      // Never mark open ocean as a shore or fill extra sprite/OAM slots.
      if (a + 2 < lb) {
        uint8_t shore = 0, accent;
        if (!h && n <= 12) {
          shore = (uint8_t)((x && hgt[y][x-1u]) ||
            (x < 31u && hgt[y][x+1u]) ||
            (y && hgt[y-1u][x]) ||
            (y < 31u && hgt[y+1u][x]));
        }
        accent = eye_surface_accent(h,x,y,n,shore);
        if (accent == 1u) cls[a] = 2;            // bright foam at water edge
        else if (accent == 2u) { cls[a] = 3; cls[a+2u] = 2; } // rock strata
        else if (accent == 3u) cls[a+1u] = 2;    // grass tuft
        else if (accent == 4u) cls[a+1u] = 3;    // stone/pebble
      }
      // One gold surface pixel reveals actual world pheromone, never a
      // decorative unrelated particle. Reuses the existing ph map, no RAM.
      if (a < VR * 8 &&
          eye_show_scent(ph[y][x], h, n,
            (uint8_t)((uint8_t)(px - 64) < 128 &&
                      (uint8_t)(py - 64) < 128)))
        cls[a] = 8;
      lim = (uint8_t)a;
    }
    o = occ[y][x];
    if (o && !(x == vx && y == vy) && eye_entity_centered((uint16_t)px, (uint16_t)py)) {
      rim = eye_entity_outer((uint16_t)px, (uint16_t)py);
      // Food is a small nugget, not a two-cell-wide yellow block.
      if (o == 5 && rim) continue;
      a = eye_entity_height(BBH[n], o);
      a = yt - a;                                    // top row of the figure
      e = yt < lb ? yt : lb;
      if (a < 0) a = 0;
      col = o == 5 ? 7 : (o & 1) ? 6 : 5;
      // Peripheral rays are only two short leg strokes, not an entire
      // vertically patterned copy of the creature. Keep the terrain
      // visible behind those gaps instead of treating them as a wall.
      if (rim) {
        if (o != 5 && e > a + 6) {
          uint8_t limb = (col == 5) ? 9 : 10;
          cls[a + 2] = limb;
          cls[e - 3] = limb;
        }
      } else for (r = a; r < e; r++) cls[r] = col;
      // Draw one tapered, segmented ant rather than repeating the old
      // body-and-legs checker every eight pixels for an oversized post.
      // Only a few actual outline rows become leg pixels; the thorax and
      // abdomen stay solid team-coloured so they read on GBC hardware.
      if (!rim && o != 5 && e > a + 4) {
        uint8_t limb = (col == 5) ? 9 : 10;
        cls[a] = limb; cls[a + 1] = limb;
        cls[e - 1] = limb;
        if (e > a + 8) {
          cls[a + 3] = limb; // feelers/forelegs
          cls[e - 3] = limb; // hind legs
        }
      }
      // Queens need a distinct silhouette: a two-pixel royal gold cap on
      // the otherwise team-colored tower. No extra columns or textures.
      if (!rim && (o == 3 || o == 4) && a < e) {
        cls[a] = 7;
        if (a + 1 < e) cls[a + 1] = 7;
      }
      if (!rim && a < lim) lim = (uint8_t)a;
    }
  }
}



/* Reuse the two already-loaded title walking-ant tiles for the live
   overhead colony. The first four hardware sprites are reserved for
   queen/player visibility; only slots 4..39 are touched. Never call
   bank-1 drawing/VRAM helpers while bank 3 is mapped. The caller disables
   banked audio interrupts and runs this at one-quarter of the frame rate.
   The idle strategy-screen text scratch holds one flag per visible ant.
   This avoids variable bit shifts and extra fixed-bank code or WRAM.
   A carrier receives tile 5/6 instead of 3/4, with its team palette.
   This saves expensive per-ant tile updates in the full fixed ROM bank. */
void eye_animate_walkers(uint8_t visible, uint8_t phase,
                         const uint8_t *carrying) BANKED {
  uint8_t i;
  for (i = 0; i < visible; ++i)
    shadow_OAM[4u + i].tile = colony_walker_frame(i, phase, carrying);
}

/* Real eight-direction ant movement lives with the renderer in bank 3,
   keeping the very full 32 KB fixed ROM area from growing. */
void eye_step(uint8_t *vx, uint8_t *vy, int8_t dir) BANKED {
  (void)eye_try_step(hgt, vx, vy, vang, dir);
}
/* Bounded nearby berry bearing: 0 if none, otherwise bit7 present,
   bits 0/1 east/west, bits 2/3 south/north. */
uint8_t eye_food_hint(uint8_t vx, uint8_t vy) BANKED {
#ifdef EYE_DIAG_FOOD_TRIVIAL
  (void)vx; (void)vy;
  return 0;
#else
  return eye_food_bearing(hgt, food, vx, vy);
#endif
}

/* A bite hits the adjacent reachable tile along the same eight-direction
   movement vector used by ANT EYE: never through cliffs or diagonal corners. */
uint8_t eye_bite_target(uint8_t vx, uint8_t vy, uint8_t *tx, uint8_t *ty) BANKED {
  return eye_attack_target(hgt, vx, vy, vang, tx, ty);
}

/* The forward context compass and bite share the exact terrain/ant data.
   It is sampled only after a change of direction or the selected worker. */
uint8_t eye_ahead(uint8_t vx, uint8_t vy) BANKED {
  return eye_ahead_kind(hgt, occ, vx, vy, vang);
}

static const uint8_t ANT_BITS[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
/* Reuse the existing 640-byte text scratch for the player's ant-eye rim.
   Each of 20 columns owns 24 one-byte masks (bottom three 8px tiles).
   This is initialized once on ANT EYE entry, not every frame or ray step.
   Cross-bank-safe: only local shifts, direct RAM writes, no libc calls. */
void eye_build_antennae(uint8_t *scratch) BANKED {
  uint8_t x, y, c, width, left, right;
  for (c = 0; c < 20; ++c)
    for (y = 0; y < 24; ++y)
      scratch[((uint16_t)c << 4) + ((uint16_t)c << 3) + y] = 0;
  for (y = 0; y < 24; ++y) {
    left  = (uint8_t)(70u - (y >> 1));
    right = (uint8_t)(89u + (y >> 1));
    width = y < 12 ? 2u : y < 20 ? 3u : 4u;
    for (c = 0; c < width; ++c) {
      x = (uint8_t)(left - c);
      scratch[((uint16_t)(x >> 3) << 4) + ((uint16_t)(x >> 3) << 3) + y] |= ANT_BITS[x & 7u];
      x = (uint8_t)(right + c);
      scratch[((uint16_t)(x >> 3) << 4) + ((uint16_t)(x >> 3) << 3) + y] |= ANT_BITS[x & 7u];
    }
    // Curved mandibles converge into the viewer's own head at the bottom.
    if (y >= 22) for (x = 57; x <= 102; ++x)
      scratch[((uint16_t)(x >> 3) << 4) + ((uint16_t)(x >> 3) << 3) + y] |= ANT_BITS[x & 7u];
  }
}
