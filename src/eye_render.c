// First-person ray marcher, kept in switchable ROM bank 3 to preserve the scarce fixed bank.
// One BANKED call updates four columns per frame. World state and constants remain in bank 0.
#pragma bank 3
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#define W 32
#define H 32
#define VX 20
#define VR 12
#define VY0 2
#define VN 24
extern uint8_t hgt[H][W], occ[H][W], cls[VR * 8], vang, tcol[VR * 16];
extern int8_t OFF[7][VN + 1];
extern const int8_t SIN16[16];
extern const uint8_t BBH[VN + 1], PMAP[3][8];

static void view_col(uint8_t c, uint8_t vx, uint8_t vy, uint8_t eh) {
  int16_t t = (int16_t)c * 2 - 19, px = (int16_t)vx * 256 + 128, py = (int16_t)vy * 256 + 128, yt, a, e, r, rx, ry;
  uint8_t n, x, y, h, o, col, lim = VR * 8, lb;
  rx = (int16_t)SIN16[(vang + 4) & 15] * 32 - (int16_t)SIN16[vang] * t;
  ry = (int16_t)SIN16[vang] * 32 + (int16_t)SIN16[(vang + 4) & 15] * t;
  rx >>= 4; ry >>= 4;                                // one step = half a cell forward
  for (n = 0; n < VR * 8; n++) cls[n] = 0;
  for (n = 1; n <= VN && lim; n++) {
    px += rx; py += ry;
    if (px < 0 || py < 0 || px >= W * 256 || py >= H * 256) break;
    x = (uint8_t)(px >> 8); y = (uint8_t)(py >> 8);
    h = hgt[y][x];
    yt = 48 - OFF[h + 3 - eh][n];
    lb = lim;
    if (yt < lb) {                                   // terrain pokes above everything nearer: paint it
      a = yt < 0 ? 0 : yt;
      col = h == 0 ? 4 : h == 3 ? 3 : ((x + y) & 1) ? 1 : 2;
      for (r = a; r < lb; r++) cls[r] = col;
      lim = (uint8_t)a;
    }
    o = occ[y][x];
    if (o && !(x == vx && y == vy) && (uint8_t)(px - 64) < 128 && (uint8_t)(py - 64) < 128) {   // centre of the cell only
      a = BBH[n]; if (o == 5) a >>= 1; else if (o > 2) a <<= 1;
      a = yt - a;                                    // top row of the post
      e = yt < lb ? yt : lb;
      if (a < 0) a = 0;
      col = o == 5 ? 7 : (o & 1) ? 6 : 5;
      for (r = a; r < e; r++) cls[r] = col;
      if (a < lim) lim = (uint8_t)a;
    }
  }
}


void view_cols(uint8_t vx, uint8_t vy, uint8_t c0, uint8_t c1) BANKED {
  uint8_t c, r, k, v, p, m, eh = hgt[vy][vx], *t;
  VBK_REG = VBK_ATTRIBUTES;                          // tile data + attributes both go to bank 1 here
  for (c = c0; c < c1; c++) {
    view_col(c, vx, vy, eh);
    t = tcol;
    for (r = 0; r < VR; r++) {
      m = 0;
      for (k = 0; k < 8; k++) { v = cls[r * 8 + k]; if (v == 4) m |= 1; else if (v > 4) m |= 2; }
      p = (m & 2) ? 2 : (m & 1) ? 1 : 0;             // palette: ants > water > plain land
      for (k = 0; k < 8; k++) { v = PMAP[p][cls[r * 8 + k]]; *t++ = (v & 1) ? 0xFF : 0; *t++ = (v & 2) ? 0xFF : 0; }
      set_win_tile_xy(c, VY0 + r, 8 | (3 + p));      // bit 3 = tile from VRAM bank 1
    }
    set_bkg_data(c * VR, VR, tcol);
  }
  VBK_REG = VBK_TILES;
}
