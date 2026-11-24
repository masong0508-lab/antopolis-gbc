/* Bounded home-defense signals for both GBC colonies.
   ONE 50-ant scan per eight simulation slices identifies the closest
   opponent within five tiles of EACH queen. Each living ant threatens
   only its opponent, so there is no need for two separate scans.
   No BFS, heap, world atlas, RNG draw or extra frame-time map scan. */
#ifndef ANTOPOLIS_COLONY_DEFENSE_H
#define ANTOPOLIS_COLONY_DEFENSE_H
#include <stdint.h>
#include "eye_occupancy.h"

static void colony_home_threats(const EyeAnt ants[EYE_OCC_ANTS],
                                 const uint8_t hx[2],
                                 const uint8_t hy[2],
                                 uint8_t nearest[2]) {
  uint8_t i, best[2] = {6u, 6u};
  nearest[0] = nearest[1] = EYE_OCC_ANTS;
  for (i = 0; i < EYE_OCC_ANTS; ++i) {
    uint8_t team, dx, dy, distance;
    if (!ants[i].alive) continue;
    team = (uint8_t)(ants[i].team ^ 1u);
    dx = ants[i].x > hx[team] ? ants[i].x - hx[team] : hx[team] - ants[i].x;
    dy = ants[i].y > hy[team] ? ants[i].y - hy[team] : hy[team] - ants[i].y;
    distance = (uint8_t)(dx + dy);
    if (distance < best[team]) {
      best[team] = distance; nearest[team] = i;
    }
  }
}
/* Guard only when close enough to actually help the nest; no map-wide
   recall that would trap the entire army behind lakes or cliffs. */
static uint8_t colony_guard_intercept(uint8_t distance_from_home,
                                      uint8_t threat_index) {
  return (uint8_t)(distance_from_home <= 8u &&
                   threat_index < EYE_OCC_ANTS);
}
#endif
