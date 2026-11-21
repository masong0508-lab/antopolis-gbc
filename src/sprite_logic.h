#ifndef ANTOPOLIS_GBC_SPRITE_LOGIC_H
#define ANTOPOLIS_GBC_SPRITE_LOGIC_H
#include <stdint.h>

// All 36 possible map workers share the original two walking poses.
// Carriers use the new two food-packet silhouettes, but keep the same
// red/black OBJ palette and the same OAM slot as ordinary workers.
// The caller supplies a FIVE-byte packed mask, built on the stack.
// No persistent WRAM state, sprite slots, or framebuffer allocations.
static uint8_t colony_walker_frame(uint8_t index, uint8_t phase,
                                    const uint8_t *carrying) {
  uint8_t frame = (uint8_t)((index + phase) & 1u);
  return (uint8_t)((carrying[index >> 3] & (uint8_t)(1u << (index & 7u)))
                     ? 5u + frame : 3u + frame);
}
#endif
