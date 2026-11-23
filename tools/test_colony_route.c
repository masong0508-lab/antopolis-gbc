/* Regression for the shipping GBC food-carrier return-home score.
   Simulates ant step selection, pheromone memory, real land/water legality,
   and the SAME random-call cadence as the previous greedy algorithm.
   No new RAM is allocated in the cartridge itself. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/colony_route.h"

#define W 32
static uint8_t terrain[W][W], scent[W][W];
static uint32_t rng;

static uint8_t md(uint8_t x,uint8_t y,uint8_t hx,uint8_t hy){
  return (uint8_t)((x>hx?x-hx:hx-x)+(y>hy?y-hy:hy-y));
}
static int walk(uint8_t x,uint8_t y,uint8_t hx,uint8_t hy,
                uint32_t seed,uint8_t anti_loop){
  static const int8_t dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
  uint16_t tick,ix,iy; uint8_t k,nx,ny;
  rng=seed;
  memset(scent,0,sizeof scent);
  for(tick=0;tick<350;tick++){
    int16_t best=-32000; uint8_t chosen=255;
    scent[y][x]=scent[y][x]>215 ? 255 : (uint8_t)(scent[y][x]+40);
    for(k=0;k<4;k++){
      int16_t px=(int16_t)x+dx[k],py=(int16_t)y+dy[k];
      int16_t score;
      if(px<0||py<0||px>=W||py>=W) continue;
      nx=(uint8_t)px;ny=(uint8_t)py;
      if(!terrain[ny][nx]||
         (int16_t)terrain[ny][nx]-terrain[y][x]>1||
         (int16_t)terrain[y][x]-terrain[ny][nx]>1) continue;
      rng=rng*1664525u+1013904223u;
      if(anti_loop)
        score=colony_home_score(md(nx,ny,hx,hy),scent[ny][nx],
                                (uint8_t)(rng>>16));
      else score=(int16_t)(-10*(int16_t)md(nx,ny,hx,hy))+
                 (int16_t)((rng>>16)&15u);
      if(score>best){best=score;chosen=k;}
    }
    if(chosen==255) return 0;
    x=(uint8_t)(x+dx[chosen]); y=(uint8_t)(y+dy[chosen]);
    if(x==hx&&y==hy) return (int)(tick+1);
    // Actual cartridge pheromone evaporates over several simulation ticks.
    if(!(tick&3u))
      for(iy=0;iy<W;iy++)for(ix=0;ix<W;ix++)
        if(scent[iy][ix]) scent[iy][ix]--;
  }
  return 0;
}
static void fresh_map(void){memset(terrain,1,sizeof terrain);}
int main(void){
  unsigned seed,x,y;
  // Clear routes remain optimal: history must not slow a direct path.
  fresh_map();
  for(seed=1;seed<=15;seed++) {
    assert(walk(4,10,27,10,seed,1)==23);
    assert(walk(4,26,4,25,seed,1)==1);
  }
  // Real water barriers. The old unmodified greedy algorithm loops
  // against each wall, whereas the existing scent field breaks the loop.
  fresh_map();
  for(y=7;y<20;y++) terrain[y][12]=0;
  for(seed=1;seed<=15;seed++){
    assert(walk(16,11,8,11,seed,0)==0);
    assert(walk(16,11,8,11,seed,1)>0);
  }
  fresh_map();
  for(x=8;x<21;x++) terrain[11][x]=0;
  for(seed=1;seed<=15;seed++){
    assert(walk(12,14,12,8,seed,0)==0);
    assert(walk(12,14,12,8,seed,1)>0);
  }
  fresh_map();
  for(x=10;x<20;x++) terrain[12][x]=0;
  for(y=8;y<13;y++) terrain[y][10]=0;
  for(seed=1;seed<=15;seed++){
    assert(walk(18,15,12,10,seed,0)==0);
    assert(walk(18,15,12,10,seed,1)>0);
  }
  // Heavy scent must defeat tempting but repeatedly failed direct steps,
  // while unscented direct movement still scores ahead of a detour.
  assert(colony_home_score(3,0,0)>colony_home_score(4,0,0));
  assert(colony_home_score(3,255,0)<colony_home_score(4,0,0));
  /* A carrier one tile from home previously avoided the heavily scented
     nest in favor of any clean detour. All 256 scent values and all 16
     random rolls MUST favor stepping onto the nest. Non-home behavior
     retains its original byte-for-byte arithmetic. */
  for (unsigned trail = 0; trail <= 255; ++trail)
    for (unsigned roll = 0; roll < 16; ++roll) {
      assert(colony_home_score(0, (uint8_t)trail, (uint8_t)roll) == 16);
      for (unsigned step = 1; step <= 62; ++step)
        assert(colony_home_score(0, (uint8_t)trail, (uint8_t)roll) >
               colony_home_score((uint8_t)step, 0, 15));
      assert(colony_home_score(3, (uint8_t)trail, (uint8_t)roll) ==
             (int16_t)(-30 + roll - (trail >> 2)));
    }
  /* Previously an empty square with scent 255 beat adjacent food with
     scent 0: 255+31 > 120. Food must now win against EVERY empty trail
     and random roll; searching on empty land keeps exactly the old score. */
  for (unsigned trail = 0; trail <= 255; ++trail)
    for (unsigned roll = 0; roll < 32; ++roll) {
      assert(colony_forager_score((uint8_t)trail, 0, (uint8_t)roll) ==
             (int16_t)(trail + roll));
      assert(colony_forager_score((uint8_t)trail, 1, (uint8_t)roll) ==
             (int16_t)(trail + roll + 288));
      assert(colony_forager_score((uint8_t)trail, 1, (uint8_t)roll) >
             colony_forager_score(255, 0, 31));
    }
  assert(colony_forager_score(0, 1, 0) > colony_forager_score(255, 0, 31));
  assert(colony_forager_score(255, 1, 0) > colony_forager_score(0, 1, 31));
  puts("GBC return-home routing + berry-first foraging: obstacle routes and all scent/roll combinations PASS");
  return 0;
}
