// First-person ray marcher, kept in switchable ROM bank 3 to preserve the scarce fixed bank.
// Only the pure per-column ray marcher is banked; VRAM writes remain in the main bank.
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
extern uint8_t hgt[H][W], occ[H][W], cls[VR * 8], vang;
extern int8_t OFF[7][VN + 1];
static const int8_t SIN16[16] = {0, 24, 45, 59, 64, 59, 45, 24, 0, -24, -45, -59, -64, -59, -45, -24};
static const uint8_t BBH[VN + 1] = {0, 40, 32, 21, 16, 12, 10, 9, 8, 7, 6, 5, 5, 4, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2};

void view_col(uint8_t c, uint8_t vx, uint8_t vy, uint8_t eh) BANKED {
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


