/* Native tests for the production GBC defensive soldier logic. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/colony_defense.h"

static EyeAnt ants[EYE_OCC_ANTS];
static const uint8_t home_x[2] = {4, 27}, home_y[2] = {26, 5};

static uint8_t threat_for(uint8_t team) {
  uint8_t nearest[2];
  colony_home_threats(ants, home_x, home_y, nearest);
  return nearest[team];
}

/* Independent original two-pass implementation for equivalence checks. */
static uint8_t reference(uint8_t team, uint8_t hx, uint8_t hy) {
  uint8_t best=6, chosen=EYE_OCC_ANTS;
  for (uint8_t i=0; i<EYE_OCC_ANTS; ++i) {
    if (!ants[i].alive || ants[i].team==team) continue;
    uint8_t dx = ants[i].x > hx ? ants[i].x-hx : hx-ants[i].x;
    uint8_t dy = ants[i].y > hy ? ants[i].y-hy : hy-ants[i].y;
    if (dx+dy<best) { best=(uint8_t)(dx+dy); chosen=i; }
  }
  return chosen;
}

int main(void) {
  uint8_t i;
  /* The shared EyeAnt declaration also brings the bank-3 occupancy
     helper into this host fixture. Exercise it so -Werror correctly
     treats the header's static function as used. */
  uint8_t berries[32][32] = {{0}};
  uint8_t queens[2] = {0, 0}, qx[2] = {4, 27}, qy[2] = {26, 5};
  memset(ants, 0, sizeof ants);
  assert(eye_occupancy_at(0, 0, ants, berries, queens, qx, qy) == 0);
  assert(threat_for(0) == EYE_OCC_ANTS);
  assert(threat_for(1) == EYE_OCC_ANTS);
  assert(!colony_guard_intercept(8, EYE_OCC_ANTS));
  assert(!colony_guard_intercept(9, 0));
  assert(colony_guard_intercept(8, 0));
  assert(colony_guard_intercept(0, 49));
  assert(!colony_guard_intercept(0, 50));

  // Respond to enemy worker or soldier entering either queen's 5-tile
  // guard radius, but do not chase distant enemies or your own workers.
  ants[0].alive = 1; ants[0].team = 0; ants[0].x = 4; ants[0].y = 26;
  ants[1].alive = 1; ants[1].team = 1; ants[1].x = 30; ants[1].y = 30;
  assert(threat_for(0) == EYE_OCC_ANTS);
  assert(threat_for(1) == EYE_OCC_ANTS);
  ants[2].alive = 1; ants[2].team = 1; ants[2].x = 9; ants[2].y = 26;
  assert(threat_for(0) == 2);
  ants[3].alive = 1; ants[3].team = 1; ants[3].x = 5; ants[3].y = 26;
  assert(threat_for(0) == 3);
  ants[3].alive = 0;
  assert(threat_for(0) == 2);
  ants[3].alive = 1; ants[3].team = 0;
  assert(threat_for(0) == 2);

  // Symmetric defense for the other colony; a black worker is an
  // intruder only near the red nest, not near its own black nest.
  ants[4].alive = 1; ants[4].team = 0; ants[4].x = 26; ants[4].y = 5;
  assert(threat_for(1) == 4);
  ants[5].alive = 1; ants[5].team = 0; ants[5].x = 27; ants[5].y = 5;
  assert(threat_for(1) == 5);
  // Threats at equal distance choose a stable lower-slot index, which
  // is important for lockstep multiplayer determinism.
  ants[6].alive = 1; ants[6].team = 1; ants[6].x = 3; ants[6].y = 26;
  ants[7].alive = 1; ants[7].team = 1; ants[7].x = 5; ants[7].y = 26;
  assert(threat_for(0) == 6);
  for (i = 8; i < EYE_OCC_ANTS; ++i) {
    ants[i].alive = 1;
    ants[i].team = (uint8_t)(i & 1u);
    ants[i].x = 16; ants[i].y = 16;
  }
  assert(threat_for(0) == 6);
  assert(threat_for(1) == 5);

  // The combined scan must choose the exact same nearest indices as
  // independent scans for both queens across dense and sparse worlds.
  uint32_t rng=0x91837u;
  for (unsigned trial=0; trial<4096; ++trial) {
    uint8_t hx[2], hy[2], actual[2];
    for (unsigned t=0; t<2; ++t) {
      rng=rng*1664525u+1013904223u; hx[t]=(uint8_t)(rng>>16)&31u;
      rng=rng*1664525u+1013904223u; hy[t]=(uint8_t)(rng>>16)&31u;
    }
    for (i=0; i<EYE_OCC_ANTS; ++i) {
      rng=rng*1664525u+1013904223u;
      ants[i].alive=(uint8_t)((rng>>24)&1u);
      ants[i].team=(uint8_t)((rng>>23)&1u);
      ants[i].x=(uint8_t)((rng>>16)&31u);
      ants[i].y=(uint8_t)((rng>>8)&31u);
    }
    colony_home_threats(ants,hx,hy,actual);
    assert(actual[0]==reference(0,hx[0],hy[0]));
    assert(actual[1]==reference(1,hx[1],hy[1]));
  }

  puts("GBC defense: one scan matches both original scans in 4096 worlds; stable ties and bounded intercept PASS");
  return 0;
}
