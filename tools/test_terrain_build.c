/* Native regression for the actual banked GBC world-generation helpers.
   This intentionally includes their shipping implementation unchanged. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define TERRAIN_BUILD_HOST_TEST 1
#include "../src/terrain_build.c"

static uint8_t h[TERRAIN_SIZE][TERRAIN_SIZE];

static void assert_walkable_neighbors(void) {
  unsigned x,y;
  for(y=0;y<TERRAIN_SIZE;y++) for(x=0;x<TERRAIN_SIZE;x++) {
    if(!h[y][x]) continue;
    if(x+1<TERRAIN_SIZE && h[y][x+1]) {
      int delta=(int)h[y][x]-(int)h[y][x+1];
      assert(delta>=-1 && delta<=1);
    }
    if(y+1<TERRAIN_SIZE && h[y+1][x]) {
      int delta=(int)h[y][x]-(int)h[y+1][x];
      assert(delta>=-1 && delta<=1);
    }
  }
}
int main(void) {
  unsigned x,y,seed,waters_before,waters_after;
  memset(h,1,sizeof h);
  terrain_bump(h,0,0,2,1);
  assert(h[0][0]==2 && h[1][1]==2 && h[2][2]==1);
  terrain_bump(h,31,31,3,1);
  assert(h[31][31]==3 && h[30][30]==3 && h[29][29]==1);
  terrain_bump(h,16,16,1,1);
  assert(h[15][15]==1 && h[17][17]==1);
  terrain_smooth(h);
  assert_walkable_neighbors();

  /* Deterministic stress across many hill and moat patterns. The solver
     may reduce cliffs but must never turn water into land (or vice versa). */
  for(seed=0;seed<50;seed++) {
    waters_before=waters_after=0;
    for(y=0;y<TERRAIN_SIZE;y++) for(x=0;x<TERRAIN_SIZE;x++) {
      unsigned m=(x*43u+y*67u+seed*19u) % 29u;
      h[y][x]=(uint8_t)(m<3u ? 0u : m<17u ? 1u : m<25u ? 2u : 3u);
      if(!h[y][x]) waters_before++;
    }
    terrain_smooth(h);
    for(y=0;y<TERRAIN_SIZE;y++) for(x=0;x<TERRAIN_SIZE;x++)
      if(!h[y][x]) waters_after++;
    assert(waters_after==waters_before);
    assert_walkable_neighbors();
  }
  // Both fresh colonies should be active immediately without relying
  // on lucky random food scatter. The supplies are rotationally balanced.
  {
    uint8_t berries[TERRAIN_SIZE][TERRAIN_SIZE] = {{0}};
    const uint8_t nests_x[2] = {4, 27};
    const uint8_t nests_y[2] = {26, 5};
    unsigned supply = 0;
    memset(h, 1, sizeof h);
    terrain_seed_nest_food(h, berries, nests_x, nests_y);
    assert(berries[26][5] && berries[25][4]);
    assert(berries[5][26] && berries[6][27]);
    assert(!berries[26][4] && !berries[5][27]);
    for(y=0;y<TERRAIN_SIZE;y++) for(x=0;x<TERRAIN_SIZE;x++)
      supply += berries[y][x] ? 1u : 0u;
    assert(supply == 4);
    terrain_seed_nest_food(h, berries, nests_x, nests_y);
    for(y=0;y<TERRAIN_SIZE;y++) for(x=0;x<TERRAIN_SIZE;x++)
      assert(berries[y][x] <= 1u);  // does not duplicate food or use RNG
    memset(berries, 0, sizeof berries);
    h[26][5] = 0; h[6][27] = 0;
    terrain_seed_nest_food(h, berries, nests_x, nests_y);
    assert(!berries[26][5] && !berries[6][27]);
    assert(berries[25][4] && berries[5][26]);
    assert(!berries[26][4] && !berries[5][27]);
    // Out-of-bounds adjacent directions are skipped safely.
    {
      const uint8_t edge_x[2] = {0,31}, edge_y[2] = {0,31};
      memset(berries, 0, sizeof berries); memset(h,1,sizeof h);
      terrain_seed_nest_food(h,berries,edge_x,edge_y);
      assert(berries[0][1] && berries[31][30]);
      assert(!berries[0][0] && !berries[31][31]);
      const uint8_t bad_x[2] = {255, 32}, bad_y[2] = {0, 0};
      memset(berries,0,sizeof berries);
      terrain_seed_nest_food(h,berries,bad_x,bad_y);
      for(y=0;y<TERRAIN_SIZE;y++) for(x=0;x<TERRAIN_SIZE;x++)
        assert(!berries[y][x]);
    }
  }
  puts("GBC banked terrain generation: borders, 50 maps, fair reachable starter food PASS");
  return 0;
}
