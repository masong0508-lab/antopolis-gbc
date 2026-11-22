#ifndef ANTOPOLIS_PALETTE_FADE_H
#define ANTOPOLIS_PALETTE_FADE_H
#include <stdint.h>
#ifndef PALETTE_HOST_TEST
#include <gb/gb.h>
#else
#define BANKED
#endif
/* Copy four faded RGB555 colors to existing RAM. Group 0..7 is gameplay
 * BG, 8..15 title BG, and 16..22 objects. Brightness is 0..8. Callers
 * guard interrupts while ROM3 is mapped and upload after returning. */
void palette_fade4(uint16_t out[4], uint8_t group, uint8_t level) BANKED;
#endif
