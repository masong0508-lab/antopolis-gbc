/* Pure return-home steering for the living GBC colony.
   A worker carrying food should prefer a nearer nest, but should not
   endlessly bounce against the same lake/cliff. The game's existing
   pheromone field is the no-allocation history of recently visited
   ground; returning workers deposit scent before picking each step.
   Penalize revisiting heavily scented cells even if those cells are
   geometrically nearer. Unloaded foragers now always notice adjacent
   berries even along a crowded pheromone trail; soldier AI is unchanged.
   No BFS buffer, new WRAM, floating point, or per-frame map scan. */
#ifndef ANTOPOLIS_COLONY_ROUTE_H
#define ANTOPOLIS_COLONY_ROUTE_H
#include <stdint.h>

static int16_t colony_home_score(uint8_t manhattan_distance,
                                 uint8_t existing_scent,
                                 uint8_t random_low_nibble) {
  return (int16_t)(-10 * (int16_t)manhattan_distance) +
         (int16_t)(random_low_nibble & 15u) -
         (int16_t)(existing_scent >> 2);
}
#endif
