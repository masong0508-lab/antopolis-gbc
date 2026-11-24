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
// A newly visible ant can reuse an OAM slot that belonged to a carrier.
// Update the TILE immediately when the silhouette type changes, without
// forcing a full bank-3 animation pass on every redraw. The periodic pass
// still advances walking poses, while this avoids 1-3 wrong-sprite frames.
static uint8_t colony_walker_needs_refresh(uint8_t shown_tile,
                                           uint8_t carrying) {
  return (uint8_t)(shown_tile < 3u || shown_tile > 6u ||
                   ((shown_tile >= 5u) != (carrying != 0u)));
}

/* The eight CGB OBJ palettes let the same seven animation tiles give
   workers, soldiers, and food carriers distinct team-aware coloring.
   Base worker palettes 0/1, cursor 2, carriers 3/4, soldiers 5/6.
   The call happens within the existing OAM property update, so no
   additional sprites or per-frame palette uploads are needed. */
static uint8_t colony_worker_palette(uint8_t team, uint8_t carrying,
                                     uint8_t soldier) {
  return (uint8_t)((team & 1u) +
                   (carrying ? 3u : soldier ? 5u : 0u));
}
#endif
