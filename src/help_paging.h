/* Scroll help by one line or one 13-line page without overshooting.
   Pure C99 helper: no persistent bytes, heap or frame-time update. */
#ifndef ANTOPOLIS_HELP_PAGING_H
#define ANTOPOLIS_HELP_PAGING_H
#include <stdint.h>
#define HELP_VISIBLE_LINES 13u
static uint8_t ant_help_next_top(uint8_t top, uint8_t lines, int8_t step) {
  uint8_t last = lines > HELP_VISIBLE_LINES ?
      (uint8_t)(lines - HELP_VISIBLE_LINES) : 0u;
  if (top > last) top = last;
  if (step < 0) {
    uint8_t amount = (uint8_t)(-step);
    return top >= amount ? (uint8_t)(top - amount) : 0u;
  }
  if (step > 0) {
    uint8_t amount = (uint8_t)step;
    return amount >= last - top ? last : (uint8_t)(top + amount);
  }
  return top;
}
#endif
