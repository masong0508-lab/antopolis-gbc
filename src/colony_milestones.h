/* Small optional single-player colony growth rewards.
 * Reuse the already-saved hatched[0] counter: no new RAM, SRAM fields,
 * timer, extra frame processing, or changes to link-cable rules. */
#ifndef ANTOPOLIS_COLONY_MILESTONES_H
#define ANTOPOLIS_COLONY_MILESTONES_H
#include <stdint.h>
static uint8_t colony_milestone_bonus(uint8_t hatch_count) {
  if (hatch_count == 8u) return 3u;
  if (hatch_count == 16u) return 4u;
  if (hatch_count == 24u) return 5u;
  return 0u;
}
#endif
