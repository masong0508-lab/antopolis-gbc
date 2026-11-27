/* Exact first-person tile occupancy after one ant or berry changes.
   Uses the same priority as the whole-world occupancy builder:
   food, ants in slot order, then queens. No new world buffers. */
#ifndef ANTOPOLIS_EYE_OCCUPANCY_H
#define ANTOPOLIS_EYE_OCCUPANCY_H
#include <stdint.h>
#define EYE_OCC_WORLD 32u
#define EYE_OCC_ANTS 50u
typedef struct { uint8_t x, y, team, alive, carry, sol; } EyeAnt;
/* Bit 3 denotes a worker carrying food. Low three bits retain the
   original 1/2 team, 3/4 queen and 5 berry codes. */
static uint8_t eye_occ_worker(const EyeAnt *a) {
  return (uint8_t)((a->team + 1u) | (a->carry ? 8u : 0u));
}

static uint8_t eye_occupancy_at(uint8_t x, uint8_t y,
                                const EyeAnt ants[EYE_OCC_ANTS],
                                uint8_t berries[32][32],
                                const uint8_t queens[2],
                                const uint8_t queens_x[2],
                                const uint8_t queens_y[2]) {
  uint8_t i, value = berries[y][x] ? 5u : 0u;
  for (i = 0; i < EYE_OCC_ANTS; i++)
    if (ants[i].alive && ants[i].x == x && ants[i].y == y)
      value = eye_occ_worker(&ants[i]);
  for (i = 0; i < 2u; i++)
    if (queens[i] && queens_x[i] == x && queens_y[i] == y)
      value = (uint8_t)(3u + i);
  return value;
}
#endif
