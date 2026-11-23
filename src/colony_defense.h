/* Bounded home-defense signals for both GBC colonies.
   A single 50-ant scan once per eight simulation slices identifies the
   closest opponent within five tiles of each queen. Nearby soldiers
   can intercept; distant squads keep their original offensive march.
   No BFS, heap, world atlas, RNG draw or extra frame-time map scan. */
#ifndef ANTOPOLIS_COLONY_DEFENSE_H
#define ANTOPOLIS_COLONY_DEFENSE_H
#include <stdint.h>
#include "eye_occupancy.h"

static uint8_t colony_home_threat(const EyeAnt ants[EYE_OCC_ANTS],
                                  uint8_t own_team,
                                  uint8_t hx, uint8_t hy) {
  uint8_t i, nearest = EYE_OCC_ANTS, best = 6u;
  for (i = 0; i < EYE_OCC_ANTS; ++i) {
    uint8_t dx, dy, distance;
    if (!ants[i].alive || ants[i].team == own_team) continue;
    dx = ants[i].x > hx ? ants[i].x - hx : hx - ants[i].x;
    dy = ants[i].y > hy ? ants[i].y - hy : hy - ants[i].y;
    distance = (uint8_t)(dx + dy);
    if (distance < best) { best = distance; nearest = i; }
  }
  return nearest;
}
/* Guard only when close enough to actually help the nest; no map-wide
   recall that would trap the entire army behind lakes or cliffs. */
static uint8_t colony_guard_intercept(uint8_t distance_from_home,
                                      uint8_t threat_index) {
  return (uint8_t)(distance_from_home <= 8u &&
                   threat_index < EYE_OCC_ANTS);
}
#endif
