#ifndef ANTOPOLIS_GBC_SPRITE_LOGIC_H
#define ANTOPOLIS_GBC_SPRITE_LOGIC_H
#include <stdint.h>

// All 36 possible map workers share the original two walking poses.
// Carriers use the new two food-packet silhouettes, but keep the same
// red/black OBJ palette and the same OAM slot as ordinary workers.
// The caller reuses the existing 640-byte UI scratch buffer for the
// first 36 byte-sized flags, not a new allocation. This is cheaper than
// emitting generic dynamic bit-shift machinery in the crowded fixed ROM.
// No persistent WRAM state, extra stack array, OAM, or framebuffer.
static uint8_t colony_walker_frame(uint8_t index, uint8_t phase,
                                    const uint8_t *carrying) {
  uint8_t frame = (uint8_t)((index + phase) & 1u);
  return (uint8_t)(carrying[index] ? 5u + frame : 3u + frame);
}
#endif
