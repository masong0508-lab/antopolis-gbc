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
  /* The atlas overlays queens last and highest ant slot last.
     Query in reverse priority, so a queen or occupied tile can return
     immediately. Walk a pointer rather than multiplying each slot by
     sizeof(EyeAnt). No cache, allocation, or visibility rule changes. */
  const EyeAnt *a = ants + EYE_OCC_ANTS;
  if (queens[1] && queens_x[1] == x && queens_y[1] == y) return 4u;
  if (queens[0] && queens_x[0] == x && queens_y[0] == y) return 3u;
  while (a != ants) {
    --a;
    if (a->alive && a->x == x && a->y == y) return eye_occ_worker(a);
  }
  return berries[y][x] ? 5u : 0u;
}
#endif
