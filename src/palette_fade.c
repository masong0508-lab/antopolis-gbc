/* Palette colors and fade arithmetic live together in ROM3. Four-color
 * batches fill the existing fixed-RAM fade buffers; the caller guards each
 * short BANKED call so pending VBlank/audio work can run between palettes.
 * No fixed-ROM pointer or library multiplication helper is used in ROM3. */
#include "palette_fade.h"
#ifndef PALETTE_HOST_TEST
#pragma bank 3
#include <gb/cgb.h>
#else
#define RGB(r,g,b) ((uint16_t)((r) | ((g) << 5) | ((b) << 10)))
#endif

// ---------- palettes & graphics (generated from strings) ----------
// Seven 4-colour CGB background palettes (0-6). Keep storage and upload
// count identical: a 24-entry declaration silently dropped the trail palette.
static const uint16_t bpal[32] = {
  RGB(5,12,25), RGB(28,23,14), RGB(9,23,8), RGB(15,11,8),  // 0: deep-water blue / sand / moss / warm rock
  RGB(2,5,13), RGB(6,11,21),  RGB(14,23,28), RGB(31,31,28), // 1: midnight blue and ivory UI
  RGB(2,5,13), RGB(14,9,5),   RGB(26,19,8),  RGB(31,29,16), // 2: warm brass headings
  RGB(18,27,31), RGB(11,26,9), RGB(27,22,12), RGB(21,14,6), // 3: luminous horizon, land + rock
  RGB(3,7,20),   RGB(8,18,30),  RGB(11,26,9),   RGB(27,22,12),   // 4: blue water beside green/tan land
  RGB(3,7,20),   RGB(31,6,4),   RGB(0,0,0),     RGB(31,27,8), // 5: ant eye, enemies and gold food
  RGB(3,7,20),   RGB(11,26,9),  RGB(27,22,12),  RGB(31,27,8), // 6: gold scent over grass and sand
  RGB(4,10,23),  RGB(7,16,28), RGB(13,22,31),  RGB(17,25,31)}; // 7: graded indigo-to-cyan sky
// Eight palette slots, with a dedicated sky gradient; no extra frame RAM.
typedef char verify_gbc_bg_palettes[(sizeof bpal / sizeof bpal[0]) == 32 ? 1 : -1];
// title screen palettes (replace bpal while the title is up): night sky in three bands, gold logo + two shine steps, hills
static const uint16_t tpal[32] = {
  RGB(1,1,5),   RGB(5,5,13),   RGB(11,11,22), RGB(31,29,22),   // 0: night sky (top)
  RGB(4,2,9),   RGB(9,6,17),   RGB(15,10,23), RGB(31,29,22),   // 1: dusk sky
  RGB(10,4,11), RGB(17,8,15),  RGB(24,13,15), RGB(31,29,22),   // 2: horizon glow
  RGB(1,1,5),   RGB(10,5,1),   RGB(22,12,3),  RGB(31,27,8),    // 3: gold logo
  RGB(10,4,11), RGB(13,26,8),  RGB(5,15,7),   RGB(9,5,4),      // 4: hills, grass, soil
  RGB(2,4,12), RGB(6,13,21),  RGB(15,23,27), RGB(31,31,27), // 5: raised title menu panel
  RGB(1,1,5),   RGB(14,8,2),   RGB(28,18,6),  RGB(31,31,16),   // 6: logo shine, soft
  RGB(1,1,5),   RGB(20,14,6),  RGB(31,26,14), RGB(31,31,28)};  // 7: logo shine, bright
// OBJ palettes 0-2 = base black/red ants and cursor. Palettes 3-4
// carry a golden berry, palettes 5-6 distinguish soldiers from foragers.
// Each recolors the SAME 7 ant sprites; only 32 extra ROM data bytes.
static const uint16_t spal[28] = {
  RGB(0,0,0), RGB(5,7,10),  RGB(19,21,25), RGB(31,31,31), // 0: black workers
  RGB(0,0,0), RGB(26,3,3),  RGB(31,12,8),  RGB(31,31,31), // 1: red workers
  RGB(0,0,0), RGB(31,31,0), RGB(31,20,0), RGB(31,31,31), // 2: cursor
  RGB(0,0,0), RGB(5,7,10),  RGB(19,21,25), RGB(31,26,8), // 3: black carrying gold
  RGB(0,0,0), RGB(26,3,3),  RGB(31,12,8),  RGB(31,26,8), // 4: red carrying gold
  RGB(0,0,0), RGB(5,11,19), RGB(17,26,31), RGB(27,31,31), // 5: black soldier steel
  RGB(0,0,0), RGB(19,3,3), RGB(31,17,7), RGB(31,28,13)  // 6: red soldier ember
};

/* The input component is five bits and level is 0..7 here. Three
 * shift/add terms reproduce floor(component * level / 8) exactly, without
 * calling SDCC's multiplication helpers from switchable ROM1. */
static uint8_t fade_component(uint8_t component, uint8_t level) {
  uint8_t total = 0;
  if (level & 1u) total = component;
  component <<= 1;
  if (level & 2u) total += component;
  component <<= 1;
  if (level & 4u) total += component;
  return total >> 3;
}
static uint16_t fade_color(uint16_t color, uint8_t level) {
  uint8_t r, g, b;
  if (!level) return 0;
  if (level == 8u) return color & 0x7FFFu;
  r = fade_component((uint8_t)(color & 31u), level);
  g = fade_component((uint8_t)((color >> 5) & 31u), level);
  b = fade_component((uint8_t)((color >> 10) & 31u), level);
  return (uint16_t)(r | ((uint16_t)g << 5) | ((uint16_t)b << 10));
}

/* group: 0..7 gameplay BG, 8..15 title BG, 16..22 object palettes.
 * level: the existing nine fade levels 0..8, inclusive. */
void palette_fade4(uint16_t out[4], uint8_t group, uint8_t level) BANKED {
  const uint16_t *source;
  uint8_t i;
  if (group < 8u) source = bpal + (group << 2);
  else if (group < 16u) source = tpal + ((group - 8u) << 2);
  else source = spal + ((group - 16u) << 2);
  for (i = 0; i < 4u; ++i) out[i] = fade_color(source[i], level);
}
