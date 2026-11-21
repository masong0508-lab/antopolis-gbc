/* One-time world-generation shaping lives in switchable ROM3 instead of
   the nearly full fixed game-code bank. No framebuffer, map allocation,
   per-tick CPU work or gameplay/save format changes. The caller guards the
   bank-1 audio ISR while each BANKED routine is active. */
#ifndef TERRAIN_BUILD_HOST_TEST
#pragma bank 3
#include <gb/gb.h>
#else
#define BANKED
#endif
#include <stdint.h>

#define TERRAIN_SIZE 32

void terrain_bump(uint8_t heights[TERRAIN_SIZE][TERRAIN_SIZE],
                  uint8_t cx, uint8_t cy, uint8_t level,
                  uint8_t radius) BANKED {
  int8_t x,y;
  for(y=(int8_t)cy-radius; y<=(int8_t)cy+radius; y++)
    for(x=(int8_t)cx-radius; x<=(int8_t)cx+radius; x++)
      if(x>=0 && y>=0 && x<TERRAIN_SIZE && y<TERRAIN_SIZE)
        heights[(uint8_t)y][(uint8_t)x]=level;
}

/* Reconcile accidental height-3/height-1 cliffs created by hill bumps.
   Water is intentionally unchanged; actual terraforming and sharp
   player-built escarpments remain possible during gameplay. */
void terrain_smooth(uint8_t heights[TERRAIN_SIZE][TERRAIN_SIZE]) BANKED {
  uint8_t pass,x,y,a,b;
  for(pass=0;pass<3;pass++)
    for(y=0;y<TERRAIN_SIZE;y++)
      for(x=0;x<TERRAIN_SIZE;x++) {
        a=heights[y][x];
        if(!a) continue;
        if(x+1<TERRAIN_SIZE) {
          b=heights[y][x+1];
          if(b) {
            if(a>b+1) heights[y][x]=b+1;
            else if(b>a+1) heights[y][x+1]=a+1;
          }
        }
        a=heights[y][x];
        if(y+1<TERRAIN_SIZE) {
          b=heights[y+1][x];
          if(b) {
            if(a>b+1) heights[y][x]=b+1;
            else if(b>a+1) heights[y+1][x]=a+1;
          }
        }
      }
}

/* Fair, immediate food for both colonies. The world generator already
   flattens a 3x3 area around each nest to dry, walkable terrain. Add
   exactly two starter berries next to each nest AFTER random scatter:
   black east+north, red west+south (rotationally symmetric). These
   positions are one legal ant move from the nest, and this function
   is never called when loading an existing save. No RNG draws, extra
   simulation ticks, new RAM, or cross-link divergence. */
void terrain_seed_nest_food(uint8_t heights[TERRAIN_SIZE][TERRAIN_SIZE],
                            uint8_t berries[TERRAIN_SIZE][TERRAIN_SIZE],
                            const uint8_t nx[2], const uint8_t ny[2]) BANKED {
  uint8_t t,x,y;
  for (t=0;t<2;t++) {
    x=nx[t]; y=ny[t];
    if (x<TERRAIN_SIZE && y<TERRAIN_SIZE) {
      if (t==0) {
        if (x+1u<TERRAIN_SIZE && heights[y][x+1u]) berries[y][x+1u]=1;
        if (y>0 && heights[y-1u][x]) berries[y-1u][x]=1;
      } else {
        if (x>0 && heights[y][x-1u]) berries[y][x-1u]=1;
        if (y+1u<TERRAIN_SIZE && heights[y+1u][x]) berries[y+1u][x]=1;
      }
    }
  }
}
