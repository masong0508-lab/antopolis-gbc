/* Differential native test for the *actual* ROM3 visibility sampling rule.
   Full world rebuild is the reference. Verify all tiles after 8,000
   movement, pickup, kill and queen transitions, including overlaps. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/eye_occupancy.h"

static EyeAnt ants[EYE_OCC_ANTS];
static uint8_t berries[32][32], seen[32][32], expected[32][32];
static uint8_t queen_hp[2], nx[2], ny[2];
static uint32_t seed = 0xA7051F43u;
static uint8_t rnd(void) {
  seed ^= seed << 13;
  seed ^= seed >> 17;
  seed ^= seed << 5;
  return (uint8_t)(seed >> 7);
}
static void rebuild(void) {
  uint8_t x,y,i;
  memset(expected,0,sizeof expected);
  for(y=0;y<32;y++)for(x=0;x<32;x++)if(berries[y][x])expected[y][x]=5u;
  for(i=0;i<EYE_OCC_ANTS;i++)if(ants[i].alive)
    expected[ants[i].y][ants[i].x]=(uint8_t)(ants[i].team+1u);
  for(i=0;i<2;i++)if(queen_hp[i])
    expected[ny[i]][nx[i]]=(uint8_t)(3u+i);
}
static void update_tile(uint8_t x,uint8_t y) {
  seen[y][x]=eye_occupancy_at(x,y,ants,berries,queen_hp,nx,ny);
}
static void verify(void) {
  uint8_t y,x;
  rebuild();
  for(y=0;y<32;y++)for(x=0;x<32;x++)assert(seen[y][x]==expected[y][x]);
}
int main(void) {
  uint32_t n;
  uint8_t i,x,y,ox,oy;
  nx[0]=4;ny[0]=26; nx[1]=27;ny[1]=5;queen_hp[0]=5;queen_hp[1]=5;
  /* Food underneath a queen or multiple workers never wins priority. */
  berries[ny[0]][nx[0]]=1;
  ants[0]=(EyeAnt){nx[0],ny[0],0,1,0,0};
  ants[1]=(EyeAnt){nx[0],ny[0],1,1,0,0};
  rebuild();memcpy(seen,expected,sizeof seen);
  assert(eye_occupancy_at(nx[0],ny[0],ants,berries,queen_hp,nx,ny)==3);
  queen_hp[0]=0;update_tile(nx[0],ny[0]);
  assert(seen[ny[0]][nx[0]]==2);
  ants[1].alive=0;update_tile(nx[0],ny[0]);
  assert(seen[ny[0]][nx[0]]==1);
  ants[0].alive=0;update_tile(nx[0],ny[0]);
  assert(seen[ny[0]][nx[0]]==5);
  berries[ny[0]][nx[0]]=0;update_tile(nx[0],ny[0]);assert(seen[ny[0]][nx[0]]==0);
  queen_hp[0]=5;
  /* Randomized legal worlds (all coordinates always in range). */
  for(i=0;i<EYE_OCC_ANTS;i++) {
    ants[i].x=(uint8_t)(rnd()&31u);ants[i].y=(uint8_t)(rnd()&31u);
    ants[i].team=(uint8_t)(rnd()&1u);ants[i].alive=(uint8_t)(rnd()&1u);
    ants[i].carry=0;ants[i].sol=0;
  }
  for(y=0;y<32;y++)for(x=0;x<32;x++)berries[y][x]=(uint8_t)(rnd()%7u==0u);
  rebuild();memcpy(seen,expected,sizeof seen);
  for(n=0;n<8000u;n++) {
    i=(uint8_t)(rnd()%EYE_OCC_ANTS);
    ox=ants[i].x; oy=ants[i].y;
    x=(uint8_t)(rnd()&31u);y=(uint8_t)(rnd()&31u);
    switch(n%6u) {
      case 0: ants[i].x=x;ants[i].y=y;break;  // old/new worker tiles
      case 1: ants[i].alive^=1u;break;       // kill/resurrect
      case 2: berries[oy][ox]^=1u;break;     // pickup or regrowth
      case 3: ants[i].team^=1u;break;       // overlap colors
      case 4: if(x==nx[1]&&y==ny[1])queen_hp[1]^=1u;
              else {x=nx[1];y=ny[1];queen_hp[1]^=1u;}break;
      default: ants[i].x=x;ants[i].y=y;berries[y][x]=0u;break;
    }
    update_tile(ox,oy);
    update_tile(ants[i].x,ants[i].y);
    if(n%6u==4u)update_tile(nx[1],ny[1]);
    verify();
  }
  /* After 8,000 actions the test exercised more than 8 million 2D
     output cells, comparing incremental visibility to a full atlas. */
  printf("PASS: 8000 GBC ANT EYE occupancy changes; all 32x32 tiles equal full rebuild\n");
  return 0;
}
