#!/usr/bin/env python3
"""Compare exact ROM3 packing to original 2bpp and palette outputs."""
from pathlib import Path
import os, re, shlex, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
render=(root/"src/eye_render.c").read_text()
main=(root/"src/main.c").read_text()
assert "eye_pack_column(c, tbuf);" in main
assert "set_win_tile_xy(c, VY0 + r, tbuf[504u + r]);" in main
assert "PMAP[" not in main
assert "eye_fill_span(0, VR * 8, 0);" in render
assert "eye_fill_span((uint8_t)a, lb, col);" in render
assert "else if (e > a) eye_fill_span((uint8_t)a, (uint8_t)e, col);" in render
def grab(pattern):
    m=re.search(pattern,render,re.S)
    assert m,pattern
    return m.group()+"\n"
fixture=r"""
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define VR 12
#define BANKED
static uint8_t cls[96], tcol[192];
"""
tables="".join(grab(r"static const uint8_t "+n+r"\[[^;]*?;") for n in ("EYE_GRAIN","EYE_BODY","EYE_LIMBS","PMAP"))
span=grab(r"static void eye_fill_span\([^)]*\) \{.*?\n\}")
pack=grab(r"void eye_pack_column\([^)]*\) BANKED \{.*?\n\}")
reference=r"""
// Precomputed 1-bit texture masks avoid a variable shift inside the
// 240-tile ANT EYE sweep; 8 ROM bytes, no RAM or extra rendering passes.
static const uint8_t OLD_EYE_GRAIN[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
// 8px silhouettes carved out of a real ant/queen ray hit. Red and black
// bodies remain team-colored; transparent gaps read as deep shadow.
// Continuous head/thorax/abdomen instead of repeated checkerboard holes.
// The two static patterns occupy the exact same 16 ROM bytes.
// Tapered 3D silhouettes: wider head, pinched thorax, rounded abdomen.
// Original ray count and 16 bytes of mask data stay exactly unchanged.
static const uint8_t OLD_EYE_BODY[8]={0x18,0x3C,0x7E,0x18,0x18,0x3C,0x7E,0x3C};
// Alternate paired leg angles instead of eight identical vertical rails.
static const uint8_t OLD_EYE_LIMBS[8]={0x81,0x42,0x81,0x24,0x81,0x42,0x81,0x42};
static const uint8_t OLD_PMAP[5][11] = {
  {0,1,2,3,0,0,0,0,0,0,0}, // land
  {0,2,3,3,1,0,0,0,0,0,0}, // water
  {0,3,3,3,0,1,2,3,0,1,2}, // ants, gold food, segmented red/black limbs
  {0,1,2,3,0,0,0,3,3,0,0}, // gold pheromone
  {0,1,2,3,0,0,0,0,0,0,0}  // upper sky
};

static void original_pack(uint8_t c, uint8_t *scratch) {
    uint8_t r,k,v,p,m,*t;
    t = tcol;
    for (r = 0; r < VR; r++) {
      m = 0;
      for (k = 0; k < 8; k++) {
        v = cls[r * 8 + k];
        if (v == 4) m |= 1;
        else if (v == 8) m |= 4;
        else if (v > 4) m |= 2;
      }
      // A separate upper atmosphere palette breaks up the old giant slab
      // of unchanging blue. Pure sky is dark above the distant horizon.
      p = (m & 2) ? 2 : (m & 4) ? 3 : (m & 1) ? 1 : 0;
      if (p == 0 && r < 3 && cls[r * 8 + 7] == 0) p = 4;
      for (k = 0; k < 8; k++) {
        uint8_t bit = OLD_EYE_GRAIN[(c + r + k) & 7u];
        uint8_t lo, hi;
        v = cls[r * 8 + k];
        lo = (OLD_PMAP[p][v] & 1u) ? 0xFFu : 0;
        hi = (OLD_PMAP[p][v] & 2u) ? 0xFFu : 0;
        // A real three-stage blue sky, rather than a black rectangle over a
        // blue rectangle. Two dithered edges soften the tile-row boundaries.
        if (p == 4 && v == 0) {
          lo = r >= 1 ? 0xFFu : (k >= 6 ? 0xAAu : 0);
          hi = r >= 2 ? 0xFFu : (r == 1 && k >= 6 ? 0xAAu : 0);
        }
        // Keep ants visibly arthropod-shaped, not solid colored posts:
        // their segmented bodies and projecting legs use the real team
        // palette and the same ray visibility/occlusion as before.
        if (p == 2 && (v == 5 || v == 6 || v == 9 || v == 10)) {
          uint8_t mask = (v >= 9) ? OLD_EYE_LIMBS[k] : OLD_EYE_BODY[k];
          lo &= mask;
          hi &= mask;
        }
        // Sparse 1-pixel rock grains, grass tufts and water ripples.
        // Only natural terrain gets them: ants and gold trails stay crisp.
        if ((p == 0 || p == 1) && (k & 3u) == 0) {
          if (v == 2) lo |= bit; // sand -> occasional brown shadow
          else if (v == 1) { lo &= (uint8_t)~bit; hi |= bit; } // grass tuft
          else if (v == 3) lo &= (uint8_t)~bit; // stone edge
          else if (v == 4) lo &= (uint8_t)~bit; // water ripple
        }
        // A stationary first-person antenna silhouette gives the 3D view
        // scale and depth without drawing more rays or sprite objects.
        if (r >= 9) {
          uint8_t ant_mask = scratch[((uint16_t)c << 4) + ((uint16_t)c << 3) + (uint8_t)(((r - 9u) << 3) + k)];
          lo |= ant_mask;
          hi |= ant_mask;
        }
        *t++ = lo; *t++ = hi;
      }
      scratch[504u + r] = (uint8_t)(8 | (3 + p));      // bit 3 = tile from VRAM bank 1
    }

}
"""
tests=r"""
static uint32_t rng=0xb4c23177u;
static uint8_t next(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return (uint8_t)rng;}
int main(void){
  uint8_t scratch[640],old[640],expected[192];
  unsigned cases=0;
  for(unsigned a=0;a<=96;a++)for(unsigned b=a;b<=96;b++){
    memset(cls,99,sizeof cls);eye_fill_span((uint8_t)a,(uint8_t)b,3);
    for(unsigned i=0;i<96;i++)assert(cls[i]==(i>=a&&i<b?3:99));
  }
  for(unsigned k=0;k<800;k++){
    for(unsigned i=0;i<96;i++)cls[i]=(uint8_t)(k<11?k:next()%11u);
    for(unsigned i=0;i<640;i++)old[i]=scratch[i]=next();
    for(unsigned c=0;c<20;c++){
      original_pack((uint8_t)c,old);memcpy(expected,tcol,sizeof expected);
      memset(tcol,0xAD,sizeof tcol);
      eye_pack_column((uint8_t)c,scratch);
      assert(!memcmp(tcol,expected,sizeof expected));
      assert(!memcmp(old,scratch,sizeof scratch));
      ++cases;
    }
  }
  // Exhaustive class pairs in each pixel position, all twenty columns.
  // This catches an early priority exit hiding a later higher class,
  // and mismatched packed low/high palette planes (including sky).
  for(unsigned a=0;a<11;a++)for(unsigned b=0;b<11;b++)for(unsigned pos=0;pos<8;pos++){
    memset(cls,(int)a,sizeof cls);
    for(unsigned r=0;r<VR;r++)cls[r*8+pos]=(uint8_t)b;
    for(unsigned c=0;c<20;c++){
      original_pack((uint8_t)c,old);memcpy(expected,tcol,sizeof expected);
      memset(tcol,0xAD,sizeof tcol);
      eye_pack_column((uint8_t)c,scratch);
      assert(!memcmp(tcol,expected,sizeof expected));
      assert(!memcmp(old,scratch,sizeof scratch));++cases;
    }
  }
  printf("GBC banked packing: %u identical columns, 4753 span bounds PASS\n",cases);
  return 0;
}
"""
with tempfile.TemporaryDirectory() as tmp:
    p=Path(tmp)/"packing.c";exe=Path(tmp)/"packing"
    p.write_text(fixture+tables+span+pack+reference+tests)
    subprocess.run(shlex.split(os.environ.get("HOSTCC","cc"))+
      ["-std=c99","-O2","-Wall","-Wextra","-Werror","-pedantic",str(p),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
