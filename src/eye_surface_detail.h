/* ANT EYE landscape material accents: read-only cosmetic decisions.
   Uses actual shore adjacency, rock strata and sparse near-field ground
   texture; ray count, physics, height map and 96px column buffer remain
   unchanged. Compiles only into switchable ROM3, not the tight fixed bank. */
#ifndef ANTOPOLIS_GBC_EYE_SURFACE_DETAIL_H
#define ANTOPOLIS_GBC_EYE_SURFACE_DETAIL_H
#include <stdint.h>
static uint8_t eye_surface_accent(uint8_t height,uint8_t x,uint8_t y,
                                  uint8_t depth,uint8_t touches_land) {
  if(depth>12u) return 0u;
  if(height==0u) return touches_land ? 1u : 0u; /* foam */
  if(height==3u) return 2u; /* lit, stratified rock edge */
  if(depth>8u) return 0u;
  // Mix high coordinate bits into the two-bit motif. Unlike x +/- y
  // parity, this breaks the uninterrupted diagonal rows of grass and
  // stones while keeping approximately one-quarter ground coverage.
  // No variable multiplication, RNG, persistent state or extra rays.
  if(height==2u) return (uint8_t)(((x ^ (y<<1) ^ (x>>2) ^ (y>>2))&3u)==0u ? 3u : 0u);
  if(height==1u) return (uint8_t)(((y ^ (x<<1) ^ (y>>2) ^ (x>>2) ^ 1u)&3u)==0u ? 4u : 0u);
  return 0u;
}
#endif
