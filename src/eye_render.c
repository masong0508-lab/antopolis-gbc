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
// Original signed ray X increment for each of 16 angles and 20 columns.
// Y is the same table rotated by -90 degrees. Integer arithmetic shifts
// are already baked into these values, including negative rounding.
// Flat indexing via shifts/adds keeps bank-3 code free of __mulint.
static const int16_t EYE_RAY_X[320] = {
  128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
  146,143,140,137,134,131,128,125,122,119,116,113,110,107,104,101,98,95,92,89,
  143,137,132,126,120,115,109,104,98,92,87,81,75,70,64,59,53,47,42,36,
  118,110,103,95,88,81,73,66,59,51,44,36,29,22,14,7,0,-8,-15,-23,
  76,68,60,52,44,36,28,20,12,4,-4,-12,-20,-28,-36,-44,-52,-60,-68,-76,
  22,14,7,-1,-8,-15,-23,-30,-37,-45,-52,-60,-67,-74,-82,-89,-96,-104,-111,-119,
  -37,-43,-48,-54,-60,-65,-71,-76,-82,-88,-93,-99,-105,-110,-116,-121,-127,-133,-138,-144,
  -90,-93,-96,-99,-102,-105,-108,-111,-114,-117,-120,-123,-126,-129,-132,-135,-138,-141,-144,-147,
  -128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,
  -147,-144,-141,-138,-135,-132,-129,-126,-123,-120,-117,-114,-111,-108,-105,-102,-99,-96,-93,-90,
  -144,-138,-133,-127,-121,-116,-110,-105,-99,-93,-88,-82,-76,-71,-65,-60,-54,-48,-43,-37,
  -119,-111,-104,-96,-89,-82,-74,-67,-60,-52,-45,-37,-30,-23,-15,-8,-1,7,14,22,
  -76,-68,-60,-52,-44,-36,-28,-20,-12,-4,4,12,20,28,36,44,52,60,68,76,
  -23,-15,-8,0,7,14,22,29,36,44,51,59,66,73,81,88,95,103,110,118,
  36,42,47,53,59,64,70,75,81,87,92,98,104,109,115,120,126,132,137,143,
  89,92,95,98,101,104,107,110,113,116,119,122,125,128,131,134,137,140,143,146
};
static int16_t eye_ray_increment(uint8_t angle, uint8_t column) {
  uint16_t row = (uint16_t)(angle & 15u);
  return EYE_RAY_X[(row << 4) + (row << 2) + column];
}
static const uint8_t BBH[VN + 1] = {0, 40, 32, 21, 16, 12, 10, 9, 8, 7, 6, 5, 5, 4, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2};

// Frozen perspective offsets derived from the original formula:
// clamp((((height - eye_height) * 4 - 3) * 16) / depth, -64, 64).
// The ray marcher already runs in this bank, so read the original ROM
// table directly rather than copying it to permanent WRAM at boot.
// Index 0 is unused and stays zero; no runtime division is introduced.
static const int8_t EYE_OFFSETS[7][25] = {
  {0,-64,-64,-64,-60,-48,-40,-34,-30,-26,-24,-21,-20,-18,-17,-16,-15,-14,-13,-12,-12,-11,-10,-10,-10},
  {0,-64,-64,-58,-44,-35,-29,-25,-22,-19,-17,-16,-14,-13,-12,-11,-11,-10,-9,-9,-8,-8,-8,-7,-7},
  {0,-64,-56,-37,-28,-22,-18,-16,-14,-12,-11,-10,-9,-8,-8,-7,-7,-6,-6,-5,-5,-5,-5,-4,-4},
  {0,-48,-24,-16,-12,-9,-8,-6,-6,-5,-4,-4,-4,-3,-3,-3,-3,-2,-2,-2,-2,-2,-2,-2,-2},
  {0,16,8,5,4,3,2,2,2,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},
  {0,64,40,26,20,16,13,11,10,8,8,7,6,6,5,5,5,4,4,4,4,3,3,3,3},
  {0,64,64,48,36,28,24,20,18,16,14,13,12,11,10,9,9,8,8,7,7,6,6,6,6}
};
// Precomputed 1-bit texture masks avoid a variable shift inside the
// 240-tile ANT EYE sweep; 8 ROM bytes, no RAM or extra rendering passes.
static const uint8_t EYE_GRAIN[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
// 8px silhouettes carved out of a real ant/queen ray hit. Red and black
// bodies remain team-colored; transparent gaps read as deep shadow.
// Continuous head/thorax/abdomen instead of repeated checkerboard holes.
// The two static patterns occupy the exact same 16 ROM bytes.
// Tapered 3D silhouettes: wider head, pinched thorax, rounded abdomen.
// Original ray count and 16 bytes of mask data stay exactly unchanged.
static const uint8_t EYE_BODY[8]={0x18,0x3C,0x7E,0x18,0x18,0x3C,0x7E,0x3C};
// Alternate paired leg angles instead of eight identical vertical rails.
static const uint8_t EYE_LIMBS[8]={0x81,0x42,0x81,0x24,0x81,0x42,0x81,0x42};
static const uint8_t PMAP[5][16] = {
  {0,1,2,3,0,0,0,0,0,0,0}, // land
  {0,2,3,3,1,0,0,0,0,0,0}, // water
  {0,3,3,3,0,1,2,3,0,1,2}, // ants, gold food, segmented red/black limbs
  {0,1,2,3,0,0,0,3,3,0,0}, // gold pheromone
  {0,1,2,3,0,0,0,0,0,0,0}  // upper sky
};
extern uint8_t tcol[VR * 16];
/* Every caller clips to the 96-pixel column. An 8-bit span count and
   advancing byte pointer avoid a signed 16-bit index per pixel. */
static void eye_fill_span(uint8_t start, uint8_t end, uint8_t color) {
  uint8_t *dst = cls + start;
  uint8_t count = (uint8_t)(end - start);
  while (count) { *dst++ = color; --count; }
}
void view_col(uint8_t c, uint8_t vx, uint8_t vy, uint8_t eh, uint8_t angle) BANKED {
  int16_t px = (int16_t)vx * 256 + 128, py = (int16_t)vy * 256 + 128, yt, a, e, rx, ry;
  uint8_t n, x, y, h, o, col, rim, carrying, lim = VR * 8, lb;
  // Same half-cell increments as the former per-column arithmetic.
  rx = eye_ray_increment(angle, c);
  ry = eye_ray_increment((uint8_t)(angle + 12u), c);
  eye_fill_span(0, VR * 8, 0);
  for (n = 1; n <= VN && lim; n++) {
    px += rx; py += ry;
    if (px < 0 || py < 0 || px >= W * 256 || py >= H * 256) break;
    x = (uint8_t)(px >> 8); y = (uint8_t)(py >> 8);
    h = hgt[y][x];
    yt = 48 - EYE_OFFSETS[h + 3 - eh][n];
    lb = lim;
    if (yt < lb) {                                   // terrain pokes above everything nearer: paint it
      a = yt < 0 ? 0 : yt;
      col = eye_ground_class(h, x, y, n);
      eye_fill_span((uint8_t)a, lb, col);
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
        else if (accent == 4u) {
          cls[a+1u] = 3;                        // pebbles on sand
          if (n <= 5 && a + 3 < lb) cls[a+2u] = 3;
        }
        // Project tiny SILHOUETTES above the actual physical ground,
        // instead of a flat color stripe at every distant horizon.
        // Only near terrain may create raised grass/rock/shore pixels.
        // Bounds are clamped before touching the shared 96px ray column.
        if (a >= 3 && n <= 8) {
          if (accent == 3u) {                   // near grass blades
            cls[a-1u] = 1;
            if (n <= 4) cls[a-2u] = 1;
          } else if (accent == 2u && n <= 6)    // chipped cliff crest
            cls[a-1u] = 3;
          else if (accent == 1u && n <= 6)      // narrow shoreline foam
            cls[a-1u] = 2;
        }
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
    carrying = o & 8u; o &= 7u;
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
      } else if (e > a) eye_fill_span((uint8_t)a, (uint8_t)e, col);
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
      // Gold is already part of the ant tile palette (class 7).
      // A food packet marks the jaws only on a visible carrier body.
      if (!rim && carrying && o <= 2u && e > a + 4) {
        cls[a + 2] = 7;
        cls[a + 3] = 7;
      }
      if (!rim && a < lim) lim = (uint8_t)a;
    }
  }
}



/* Pack the 96 classified ray pixels into the original 192-byte
   2bpp column and 12 palette attributes. Row pointers and a padded
   16-byte palette stride avoid repeated indexing and multiplication.
   Run in ROM3 with audio IRQs masked; caller alone writes VRAM. */
void eye_pack_column(uint8_t c, uint8_t *scratch) BANKED {
  uint8_t r, k, v, p, m, color, lo, hi;
  uint8_t *t = tcol, *attrs = scratch + 504u;
  const uint8_t *row = cls, *palette;
  const uint8_t *antennae = scratch + ((uint16_t)c << 4) + ((uint16_t)c << 3);
  for (r = 0; r < VR; ++r, row += 8u) {
    m = 0;
    for (k = 0; k < 8u; ++k) {
      v = row[k];
      if (v == 4) m |= 1;
      else if (v == 8) m |= 4;
      else if (v > 4) m |= 2;
    }
    p = (m & 2) ? 2 : (m & 4) ? 3 : (m & 1) ? 1 : 0;
    if (p == 0 && r < 3 && row[7] == 0) p = 4;
    palette = &PMAP[0][0] + ((uint16_t)p << 4);
    for (k = 0; k < 8u; ++k) {
      v = row[k]; color = palette[v];
      lo = (color & 1u) ? 0xFFu : 0;
      hi = (color & 2u) ? 0xFFu : 0;
      if (p == 4 && v == 0) {
        lo = r >= 1 ? 0xFFu : (k >= 6 ? 0xAAu : 0);
        hi = r >= 2 ? 0xFFu : (r == 1 && k >= 6 ? 0xAAu : 0);
      }
      if (p == 2 && (v == 5 || v == 6 || v == 9 || v == 10)) {
        uint8_t mask = (v >= 9) ? EYE_LIMBS[k] : EYE_BODY[k];
        lo &= mask; hi &= mask;
      }
      if ((p == 0 || p == 1) && (k & 3u) == 0) {
        uint8_t bit = EYE_GRAIN[(c + r + k) & 7u];
        if (v == 2) lo |= bit;
        else if (v == 1) { lo &= (uint8_t)~bit; hi |= bit; }
        else if (v == 3 || v == 4) lo &= (uint8_t)~bit;
      }
      if (r >= 9) { uint8_t ant_mask = *antennae++; lo |= ant_mask; hi |= ant_mask; }
      *t++ = lo; *t++ = hi;
    }
    *attrs++ = (uint8_t)(8 | (3 + p));
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
