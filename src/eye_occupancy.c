/* Event-driven 3D occupancy refresh in spacious ROM bank 3.
   Calling fixed-bank code disables audio IRQs during this bank switch.
   First-person movement changes at most two world tiles per poll;
   do not re-scan all 1,024 food cells and clear 1,024 occupancy bytes. */
#pragma bank 3
#include <gb/gb.h>
#include <stdint.h>
#include "eye_occupancy.h"

extern EyeAnt ant[EYE_OCC_ANTS];
extern uint8_t food[32][32], occ[32][32];
extern uint8_t qhp[2], nestx[2], nesty[2];

/* A full atlas is needed only on mode entry. Overwrite every tile with
   the true food state (zero included), then overlay the original stable
   ant/queen priority. Doing this in ROM3 frees critical fixed CODE and
   removes the redundant 1,024-byte memset before the full food scan. */
void eye_occ_init(void) BANKED {
  uint8_t x, y, i;
  for (y = 0; y < 32u; ++y)
    for (x = 0; x < 32u; ++x)
      occ[y][x] = food[y][x] ? 5u : 0u;
  for (i = 0; i < EYE_OCC_ANTS; ++i)
    if (ant[i].alive) occ[ant[i].y][ant[i].x] = eye_occ_worker(&ant[i]);
  for (i = 0; i < 2u; ++i)
    if (qhp[i]) occ[nesty[i]][nestx[i]] = (uint8_t)(3u + i);
}

void eye_occ_pair(uint8_t oldx, uint8_t oldy,
                  uint8_t newx, uint8_t newy) BANKED {
  occ[oldy][oldx] = eye_occupancy_at(oldx, oldy, ant, food, qhp, nestx, nesty);
  if (oldx != newx || oldy != newy)
    occ[newy][newx] = eye_occupancy_at(newx, newy, ant, food, qhp, nestx, nesty);
}
