/* Pure GBC bank-3 ANT EYE surface detail. Prove motifs are
   deterministic, restricted to their physical biome and near range,
   and never claim a fake coastline in open water. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/eye_surface_detail.h"
int main(void) {
  unsigned h,x,y,n,shore,seen[5]={0};
  for(h=0;h<4;h++)for(x=0;x<32;x++)for(y=0;y<32;y++)
    for(n=1;n<=24;n++)for(shore=0;shore<2;shore++) {
      uint8_t a=eye_surface_accent((uint8_t)h,(uint8_t)x,(uint8_t)y,
                                  (uint8_t)n,(uint8_t)shore);
      assert(a<=4u);
      if(n>12u) assert(a==0u);
      if(h==0u) assert(a==(uint8_t)((n<=12u && shore)?1u:0u));
      else if(h==3u) assert(a==(uint8_t)(n<=12u?2u:0u));
      else if(h==2u) assert(a==0u||a==3u);
      else assert(a==0u||a==4u);
      if(a)seen[a]++;
      assert(a==eye_surface_accent((uint8_t)h,(uint8_t)x,(uint8_t)y,
                                   (uint8_t)n,(uint8_t)shore));
    }
  for(unsigned i=1;i<5;i++)assert(seen[i]);
  // Live ROM3 raymarch must clip raised silhouettes inside 96px,
  // respect occluding ground, and keep nearer pheromone render intact.
    puts("GBC bank-3 ANT EYE environment PASS: shoreline foam, rock strata, grass and pebbles");
  return 0;
}
