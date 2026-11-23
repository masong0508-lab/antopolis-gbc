/* Small optional single-player colony growth rewards.
 * Reuse the already-saved hatched[0] counter: no new RAM, SRAM fields,
 * timer, extra frame processing, or changes to link-cable rules. */
#ifndef ANTOPOLIS_COLONY_MILESTONES_H
#define ANTOPOLIS_COLONY_MILESTONES_H
#include <stdint.h>
static uint8_t colony_milestone_bonus(uint8_t hatch_count) {
  /* Only 8, 16 or 24 qualify. An unsigned-byte range test
     replaces three equality branches; all 256 counts are host-tested. */
  if ((uint8_t)(hatch_count - 8u) > 16u || (hatch_count & 7u)) return 0u;
  return (uint8_t)((hatch_count >> 3) + 2u);
}
#endif
