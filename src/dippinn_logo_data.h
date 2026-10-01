#pragma once
// Generated artwork (tools/make_dippinn_logo.py). Internal to the logo module.
extern const unsigned int dl_sc_tile_count, dl_tx_tile_count;
extern const unsigned char dl_sc_tiles[], dl_sc_map[];   // scene: map 32 x 18 (attributes: cols <13 = band palette row/3, else 7)
extern const unsigned char dl_sc_cslot[], dl_sc_ncol;    // palette slot per colour (bit 7 = sprite palette RAM)
extern const unsigned short dl_sc_ccol[];
extern const unsigned char dl_rot_base[];                // 6 noise tiles (1..6) that the code rotates a pixel per frame
extern const unsigned char dl_spr_tiles[];               // 36 sprite tiles at 128..163
extern const unsigned char dl_gr_tiles[];
extern const unsigned short dl_gr_pal[4];
extern const unsigned char dl_tx_tiles[], dl_tx_map[], dl_rope_pat[];  // tx_map 20 x 2, rope pattern 4 rows x 4 px
extern const unsigned short dl_tx_pal[8];
extern const signed char dl_sin_q7[256];
