/* Safe no-allocation nest warp for the existing map cursor.
 * 1 = own nest, 2 = rival nest, 0 = unchanged. Does not teleport ants,
 * terraform tiles, consume mana or modify battery-save structures. */
#ifndef ANTOPOLIS_COLONY_CURSOR_H
#define ANTOPOLIS_COLONY_CURSOR_H
#include <stdint.h>
static uint8_t colony_cursor_warp(uint8_t destination, uint8_t team,
                                  const uint8_t nest_x[2],
                                  const uint8_t nest_y[2],
                                  uint8_t *cursor_x, uint8_t *cursor_y) {
  uint8_t target;
  if (team > 1 || (destination != 1 && destination != 2)) return 0;
  target = (destination == 1) ? team : (uint8_t)(team ^ 1u);
  if (nest_x[target] >= 32 || nest_y[target] >= 32) return 0;
  *cursor_x = nest_x[target];
  *cursor_y = nest_y[target];
  return 1;
}
#endif
