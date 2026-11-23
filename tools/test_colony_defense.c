/* Native tests for the production GBC defensive soldier logic. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/colony_defense.h"

static EyeAnt ants[EYE_OCC_ANTS];

int main(void) {
  uint8_t i;
  memset(ants, 0, sizeof ants);
  assert(colony_home_threat(ants, 0, 4, 26) == EYE_OCC_ANTS);
  assert(colony_home_threat(ants, 1, 27, 5) == EYE_OCC_ANTS);
  assert(!colony_guard_intercept(8, EYE_OCC_ANTS));
  assert(!colony_guard_intercept(9, 0));
  assert(colony_guard_intercept(8, 0));
  assert(colony_guard_intercept(0, 49));
  assert(!colony_guard_intercept(0, 50));

  // Respond to enemy worker or soldier entering either queen's 5-tile
  // guard radius, but do not chase distant enemies or your own workers.
  ants[0].alive = 1; ants[0].team = 0; ants[0].x = 4; ants[0].y = 26;
  ants[1].alive = 1; ants[1].team = 1; ants[1].x = 30; ants[1].y = 30;
  assert(colony_home_threat(ants, 0, 4, 26) == EYE_OCC_ANTS);
  assert(colony_home_threat(ants, 1, 27, 5) == EYE_OCC_ANTS);
  ants[2].alive = 1; ants[2].team = 1; ants[2].x = 9; ants[2].y = 26;
  assert(colony_home_threat(ants, 0, 4, 26) == 2);
  ants[3].alive = 1; ants[3].team = 1; ants[3].x = 5; ants[3].y = 26;
  assert(colony_home_threat(ants, 0, 4, 26) == 3);
  ants[3].alive = 0;
  assert(colony_home_threat(ants, 0, 4, 26) == 2);
  ants[3].alive = 1; ants[3].team = 0;
  assert(colony_home_threat(ants, 0, 4, 26) == 2);

  // Symmetric defense for the other colony; a black worker is an
  // intruder only near the red nest, not near its own black nest.
  ants[4].alive = 1; ants[4].team = 0; ants[4].x = 26; ants[4].y = 5;
  assert(colony_home_threat(ants, 1, 27, 5) == 4);
  ants[5].alive = 1; ants[5].team = 0; ants[5].x = 27; ants[5].y = 5;
  assert(colony_home_threat(ants, 1, 27, 5) == 5);
  // Threats at equal distance choose a stable lower-slot index, which
  // is important for lockstep multiplayer determinism.
  ants[6].alive = 1; ants[6].team = 1; ants[6].x = 3; ants[6].y = 26;
  ants[7].alive = 1; ants[7].team = 1; ants[7].x = 5; ants[7].y = 26;
  assert(colony_home_threat(ants, 0, 4, 26) == 6);
  for (i = 8; i < EYE_OCC_ANTS; ++i) {
    ants[i].alive = 1;
    ants[i].team = (uint8_t)(i & 1u);
    ants[i].x = 16; ants[i].y = 16;
  }
  assert(colony_home_threat(ants, 0, 4, 26) == 6);
  assert(colony_home_threat(ants, 1, 27, 5) == 5);

  puts("GBC defense: both queens, closest threats, stable ties and bounded soldier intercept PASS");
  return 0;
}
