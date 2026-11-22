/* Execute the ROM3 palette code itself. Preserve the original RGB555
 * arithmetic and all 92 authored colors, not just a few screenshots. */
#define PALETTE_HOST_TEST
#include "../src/palette_fade.c"
#include <assert.h>
#include <stdio.h>

static uint16_t reference(uint16_t color, uint8_t level) {
  uint16_t r = (uint16_t)(((color & 31u) * level) >> 3);
  uint16_t g = (uint16_t)((((color >> 5) & 31u) * level) >> 3);
  uint16_t b = (uint16_t)((((color >> 10) & 31u) * level) >> 3);
  return (uint16_t)(r | (g << 5) | (b << 10));
}

int main(void) {
  uint32_t color, fingerprint = 2166136261u;
  uint8_t level, group, i;
  uint16_t full[92], output[6];

  /* All encoded inputs, including an unused high bit, and all nine
   * existing brightness levels must match the previous multiplication. */
  for (color = 0; color < 65536u; ++color)
    for (level = 0; level <= 8u; ++level)
      assert(fade_color((uint16_t)color, level) == reference((uint16_t)color, level));

  /* Golden fingerprint of the previous release's gameplay, title and
   * object RGB words, in that order, serialized low byte first. It also
   * catches palette selection/order mistakes when moving the tables. */
  for (group = 0; group < 23u; ++group) {
    palette_fade4(full + group * 4u, group, 8);
    for (i = 0; i < 4u; ++i) {
      uint16_t word = full[group * 4u + i];
      fingerprint = (fingerprint ^ (word & 255u)) * 16777619u;
      fingerprint = (fingerprint ^ (word >> 8)) * 16777619u;
    }
  }
  assert(fingerprint == 0xAE2785A5u);

  /* Every real hardware palette at every brightness, with both ends of
   * the caller's four-word buffer protected against accidental writes. */
  for (group = 0; group < 23u; ++group)
    for (level = 0; level <= 8u; ++level) {
      output[0] = 0x55AAu; output[5] = 0xA55Au;
      for (i = 1; i < 5u; ++i) output[i] = 0xFFFFu;
      palette_fade4(output + 1, group, level);
      assert(output[0] == 0x55AAu && output[5] == 0xA55Au);
      for (i = 0; i < 4u; ++i)
        assert(output[i + 1] == reference(full[group * 4u + i], level));
    }
  puts("GBC palette fade PASS: 589824 RGB/level cases, 92 original colors, 207 bounded palette batches");
  return 0;
}
