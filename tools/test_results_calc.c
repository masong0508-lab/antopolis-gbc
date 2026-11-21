/* Compile and exercise the EXACT shipped bank-3 match arithmetic.
   Check its output against the former fixed-ROM arithmetic over many
   times, score levels, colony sizes, popularities and royal healths. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define RESULTS_HOST_TEST 1
#include "../src/results_calc.c"

static void expect(uint16_t ticks, uint8_t diff,
                   uint8_t ants, uint8_t approval, uint8_t hp) {
  uint16_t secs, points, wantsecs, wantpoints;
  uint32_t converted=(uint32_t)ticks*2u/15u;
  wantsecs=(uint16_t)(converted>5999u?5999u:converted?converted:1u);
  wantpoints=(uint16_t)(300u*(diff+1u));
  if(wantsecs<600u) wantpoints+=(uint16_t)(600u-wantsecs);
  wantpoints+=(uint16_t)(ants*5u+approval+hp*10u);
  secs=points=0xFFFFu;
  results_compute(ticks,diff,ants,approval,hp,&secs,&points);
  assert(secs==wantsecs);
  assert(points==wantpoints);
}
int main(void) {
  uint32_t t;
  const uint16_t boundaries[]={0,1,7,8,14,15,22,23,149,150,151,
    4498,4499,4500,4501,44998,44999,45000,45001,65535};
  for(unsigned b=0;b<sizeof boundaries/sizeof boundaries[0];++b)
    for(uint8_t d=0;d<3;++d) for(uint8_t hp=0;hp<8;++hp)
      expect(boundaries[b],d,25,99,hp);
  for(t=0;t<65536u;t+=37u)
    for(uint8_t d=0;d<3;++d)
      expect((uint16_t)t,d,(uint8_t)((t>>3u)%26u),
             (uint8_t)(t%100u),(uint8_t)((t>>5u)%8u));
  // Independent of the score output pointer's starting contents.
  expect(65535,2,25,99,7);
  puts("GBC bank-3 match time/score: exact original formula, all level and rollover boundaries PASS");
  return 0;
}
