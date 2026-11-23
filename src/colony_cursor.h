/* GBC nest cursor targeting: no allocations or save-format changes.
 * All gameplay nest coordinates are already bounded by world setup/save
 * validation; the player index is 0 for black, 1 for red. */
#ifndef ANTOPOLIS_COLONY_CURSOR_H
#define ANTOPOLIS_COLONY_CURSOR_H
#include <stdint.h>
static uint8_t colony_cursor_target(uint8_t team, uint8_t rival) {
  return (uint8_t)((team & 1u) ^ (rival & 1u));
}
#endif
