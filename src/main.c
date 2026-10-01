// EMPIRE-ANTS - Populous x SimAnt x Tropico for Game Boy Color (GBDK-2020)
// 32x32 scrolling world, queen ants, sound. All graphics generated in code.
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <rand.h>
#include "dippinn_logo.h"

#define W 32            // world size = size of the hardware BG map
#define H 32
#define VW 20           // visible world tiles (bottom 2 rows are the HUD window)
#define VH 16
#define MAXA 34         // workers (queens + cursor sprites come after these)
#define QHP 5
#define MANA_MAX 20     // mana cap
#define MANA_TRICKLE 32 // passive: +1 mana every this many ticks
#define MANA_PER_FOOD 1 // tribute: mana per food a black ant carries home
#define OFFER_FOOD 2    // embezzle (SELECT+A): food spent ...
#define OFFER_MANA 4    // ... for this much mana
#define BT 128          // first terrain tile
#define BAR_F 134       // mana bar tiles (free slots after the 6 terrain tiles)
#define BAR_E 135
#define TT 176          // first title-logo tile (13 tiles of 3D lettering)
#define FT 136          // first font tile (40 glyphs)

typedef struct { uint8_t x, y, team, alive, carry, sol; } Ant;

static uint8_t hgt[H][W], food[H][W], ph[H][W];
static Ant ant[MAXA];
static uint8_t nestx[2], nesty[2], stock[2], qhp[2], ncnt[2], hatched[2];
static uint8_t cx, cy, mana, tk, over, camx, camy, sandbox, cheat, ff, appr;       // appr = popularity 0-99
static uint16_t etk;                           // ticks since the last election
static const char *msg; static uint8_t msgt;      // short HUD hint (replaces the QUEEN row for ~1.5 s)
static int16_t scx, scy;                       // pixel scroll
static uint16_t seed;
static uint8_t tut, tev, tutor, lcx, lcy;      // tutorial: lesson (0 = off), events the player did, from-title flag, last cursor
// Konami code (in play): infinite mana + food. Ends on A, so it never collides with START (pause).
static const uint8_t KONAMI[10] = {J_UP, J_UP, J_DOWN, J_DOWN, J_LEFT, J_RIGHT, J_LEFT, J_RIGHT, J_B, J_A};
static const int8_t DX[4] = {1, -1, 0, 0};
static const int8_t DY[4] = {0, 0, 1, -1};

// ---------- palettes & graphics (generated from strings) ----------
static const palette_color_t bpal[24] = {
  RGB(8,16,28), RGB(28,24,14), RGB(8,20,6), RGB(6,4,3),      // 0: terrain
  RGB(0,0,0),   RGB(8,8,8),    RGB(20,20,20), RGB(31,31,31), // 1: HUD text
  RGB(0,0,0),   RGB(10,5,1),   RGB(22,12,3),  RGB(31,27,8),   // 2: gold title logo
  RGB(3,7,20),   RGB(11,26,9),  RGB(4,16,6),    RGB(21,14,6),    // 3: ant eye, land + rock
  RGB(3,7,20),   RGB(8,18,30),  RGB(11,26,9),   RGB(4,16,6),     // 4: ant eye, water
  RGB(3,7,20),   RGB(31,6,4),   RGB(0,0,0),     RGB(4,16,6)};    // 5: ant eye, red / black ants
static const palette_color_t spal[12] = {
  RGB(0,0,0), RGB(2,2,2),   RGB(14,14,14), RGB(31,31,31),
  RGB(0,0,0), RGB(26,3,3),  RGB(31,12,8),  RGB(31,31,31),
  RGB(0,0,0), RGB(31,31,0), RGB(31,20,0),  RGB(31,31,31)};

// 0 water,1 sand,2 grass,3 hill,4 nest,5 food
static const char *const BG[6][8] = {
 {"00000000","00010000","00000000","00000100","00000000","01000000","00000000","00000010"},
 {"11111111","11111111","11131111","11111111","11111111","11111311","11111111","11111111"},
 {"22222222","22122222","22222222","22222212","22222222","21222222","22222222","22222122"},
 {"22222222","22233222","22333322","23333332","33333333","33333333","23333332","22222222"},
 {"11111111","11333311","13333331","13333331","13333331","13333331","11333311","11111111"},
 {"22222222","22211222","22111122","22111122","22111122","22211222","22222222","22222222"}};
static const char *const BAR[2][8] = {
 {"00000000","00000000","33333333","33333333","33333333","33333333","00000000","00000000"},
 {"00000000","00000000","11111111","11111111","11111111","11111111","00000000","00000000"}};
// 0 worker ant, 1 cursor, 2 queen
static const char *const SP[3][8] = {
 {"00000000","00200200","00022000","00111100","01111110","00111100","00200200","02000020"},
 {"11100111","10000001","10000001","00000000","00000000","10000001","10000001","11100111"},
 {"03030300","00333000","00122100","01111110","11111111","01111110","00122100","00200200"}};

// 3x5 font, one octal digit per row (4=left px, 2=middle, 1=right)
// order: space, 0-9, A-Z, ':', '!', '/'
static const uint16_t FONT[40] = {
  0,
  075557,026227,071747,071717,055711,074717,074757,071122,075757,075717,
  025755,065656,034443,065556,074647,074644,034553,055755,072227,011152,
  055655,044447,057755,065555,025552,065644,025563,065655,034216,072222,
  055557,055552,055775,055255,055222,071247,
  002020,022202,011244};

static void mk(uint8_t *d, const char *const *r) {
  uint8_t y, x, lo, hi, c;
  for (y = 0; y < 8; y++) {
    lo = hi = 0;
    for (x = 0; x < 8; x++) { c = r[y][x] - '0'; lo = (lo << 1) | (c & 1); hi = (hi << 1) | (c >> 1); }
    d[y * 2] = lo; d[y * 2 + 1] = hi;
  }
}

static void mk_glyph(uint8_t *d, uint16_t g) {
  uint8_t y, lo;
  for (y = 0; y < 8; y++) {
    lo = (y >= 1 && y <= 5) ? (uint8_t)(((g >> (3 * (5 - y))) & 7) << 3) : 0;
    d[y * 2] = lo; d[y * 2 + 1] = lo;          // both planes = colour 3
  }
}

// ---------- text on the window layer ----------
static uint8_t gi(char c) {
  if (c >= '0' && c <= '9') return 1 + (c - '0');
  if (c >= 'A' && c <= 'Z') return 11 + (c - 'A');
  if (c == ':') return 37;
  if (c == '!') return 38;
  if (c == '/') return 39;
  return 0;
}
static void put_char(uint8_t x, uint8_t y, char c) { set_win_tile_xy(x, y, FT + gi(c)); }
static void put_str(uint8_t x, uint8_t y, const char *s) { while (*s) put_char(x++, y, *s++); }
static void put_num(uint8_t x, uint8_t y, uint8_t n) {
  if (n > 99) n = 99;
  put_char(x, y, '0' + n / 10); put_char(x + 1, y, '0' + n % 10);
}
// ---------- 3D "perspective" title lettering: EmpIre-antS ----------
// 3x5 font glyphs stretched 2x wide, extruded 2 px down-right in two darker shades (gold -> brown -> dark);
// the first E and last S are double height so the word reads Empire-antS with towering end caps.
static uint8_t cv[16][8];
static void mk_3d(uint8_t *d, uint16_t g, uint8_t tall, uint8_t part) {
  uint8_t gx, gy, k, x, y, r, c, v, lo, hi, sy = tall ? 2 : 1, y0 = tall ? 3 : 8;
  for (r = 0; r < 16; r++) for (c = 0; c < 8; c++) cv[r][c] = 0;
  for (k = 3; k--; )                                    // extrusion layers 2, 1 then front face 0 (back to front)
    for (gy = 0; gy < 5; gy++) for (gx = 0; gx < 3; gx++)
      if ((g >> (3 * (4 - gy))) & (4 >> gx))
        for (y = 0; y < sy; y++) for (x = 0; x < 2; x++) cv[y0 + gy * sy + y + k][gx * 2 + x + k] = 3 - k;
  for (r = 0; r < 8; r++) {
    lo = hi = 0;
    for (c = 0; c < 8; c++) { v = cv[part * 8 + r][c]; lo = (lo << 1) | (v & 1); hi = (hi << 1) | (v >> 1); }
    d[r * 2] = lo; d[r * 2 + 1] = hi;
  }
}
static void title_tiles(uint8_t *b) {                   // 13 tiles: E top/bottom, S top/bottom, then M P I R E - A N T
  static const char SM[10] = "MPIRE-ANT";
  uint8_t i;
  mk_3d(b,      FONT[gi('E')], 1, 0); mk_3d(b + 16, FONT[gi('E')], 1, 1);
  mk_3d(b + 32, FONT[gi('S')], 1, 0); mk_3d(b + 48, FONT[gi('S')], 1, 1);
  for (i = 0; i < 9; i++) mk_3d(b + (4 + i) * 16, SM[i] == '-' ? 0700 : FONT[gi(SM[i])], 0, 1);
}
static void logo_cells(uint8_t x0, uint8_t y0, uint8_t pal) {
  uint8_t i;
  VBK_REG = VBK_ATTRIBUTES;
  for (i = 0; i < 11; i++) { set_win_tile_xy(x0 + i, y0, pal); set_win_tile_xy(x0 + i, y0 + 1, pal); }
  VBK_REG = VBK_TILES;
}
static void title_logo(uint8_t x0, uint8_t y0) {
  static const uint8_t R2[11] = {1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 3};   // bottom row: E, M P I R E - A N T, S
  uint8_t i;
  for (i = 0; i < 11; i++) { set_win_tile_xy(x0 + i, y0, FT); set_win_tile_xy(x0 + i, y0 + 1, TT + R2[i]); }
  set_win_tile_xy(x0, y0, TT); set_win_tile_xy(x0 + 10, y0, TT + 2);
  logo_cells(x0, y0, 2);                                // gold palette
}

static void help_draw(void) {          // lives on window rows 3-17; pausing slides the window up to cover the screen
  put_str(7, 3, "PAUSED");
  put_str(1, 5, "A   RAISE LAND");
  put_str(1, 6, "B   LOWER LAND");
  put_str(1, 7, "SEL FLOOD 3X3 8MP");
  put_str(1, 8, "SEL A EMBEZZLE FOOD");
  put_str(1, 9, "SEL B FAST FORWARD");
  put_str(1, 10, "START PAUSE HELP");
  put_str(1, 11, "SEL START ANT EYE");
  put_str(1, 12, "KILL THE RED QUEEN");
  put_str(1, 13, "P = POPULARITY");
  put_str(0, 14, "ELECTION EVERY 1 MIN");
  put_str(1, 15, "HIGH P WINS AID");
  put_str(1, 16, "LOW P MEANS COUP");
  put_str(2, 17, "START TO RESUME");
}
static void win_clear(void) {
  uint8_t x, y;
  for (y = 0; y < 18; y++) for (x = 0; x < 20; x++) set_win_tile_xy(x, y, FT);
}
static void say(const char *m) { msg = m; msgt = 12; }
static uint8_t hdirty = 1, hlab, hm = 255, hs = 255, hq0 = 255, hq1 = 255, ha = 255, hr = 255, hf = 255, hp = 255;
static void hud(void) {                          // redraws only what changed: window writes are slow, so no full redraw
  uint8_t i;
  if (hdirty) {                                  // new game: static labels + force every number
    put_str(0, 0, "MP:"); put_str(16, 0, "F:");
    hm = hs = hq0 = hq1 = ha = hr = hf = hp = 255; hlab = 0; hdirty = 0;
  }
  if (mana != hm) {
    hm = mana; put_num(3, 0, mana);
    for (i = 0; i < 10; i++) set_win_tile_xy(5 + i, 0, mana > 2 * i ? BAR_F : BAR_E);   // mana bar, 2 mana per cell
  }
  if (stock[0] != hs) { hs = stock[0]; put_num(18, 0, hs); }
  if (ff != hf) { hf = ff; put_char(15, 0, ff ? 'X' : ' '); }   // X = fast forward on
  if (msgt) {                                   // hint overlay: text padded to the full 20 columns
    uint8_t x = 0; const char *m = msg;
    while (x < 20) put_char(x++, 1, *m ? *m++ : ' ');
    msgt--; hlab = 0;                           // bottom row labels must be redrawn afterwards
    return;
  }
  if (!hlab) {                                  // Q:5/5 A:NN R:NN P:NN  (queens, ants, red ants, popularity)
    put_str(0, 1, "Q:"); put_char(3, 1, '/'); put_str(5, 1, " A:"); put_str(10, 1, " R:"); put_str(15, 1, " P:");
    hq0 = hq1 = ha = hr = hp = 255; hlab = 1;
  }
  if (qhp[0] != hq0) { hq0 = qhp[0]; put_char(2, 1, '0' + hq0); }
  if (qhp[1] != hq1) { hq1 = qhp[1]; put_char(4, 1, '0' + hq1); }
  if (ncnt[0] != ha) { ha = ncnt[0]; put_num(8, 1, ha); }
  if (ncnt[1] != hr) { hr = ncnt[1]; put_num(13, 1, hr); }
  if (appr != hp) { hp = appr; put_num(18, 1, hp); }
}

// ---------- sound ----------
#define N_C4 1548
#define N_G4 1714
#define N_C5 1797
#define N_E5 1849
#define N_G5 1881
#define N_C6 1923
static const uint16_t WIN_TUNE[6]  = {N_C5, N_E5, N_G5, N_C6, N_G5, N_C6};
static const uint16_t LOSE_TUNE[4] = {N_G5, N_E5, N_C5, N_C4};
static const uint16_t *tune_p;
static uint8_t jl, ji, jt;

static uint8_t h1, h2, hn;   // music yields a channel for a few frames after a sound effect uses it
static void ch1(uint8_t sweep, uint16_t f, uint8_t env) {
  h1 = 14;
  NR10_REG = sweep; NR11_REG = 0x80; NR12_REG = env;
  NR13_REG = (uint8_t)f; NR14_REG = 0x80 | (uint8_t)(f >> 8);
}
static void ch2(uint16_t f, uint8_t env) {
  h2 = 12;
  NR21_REG = 0x80; NR22_REG = env;
  NR23_REG = (uint8_t)f; NR24_REG = 0x80 | (uint8_t)(f >> 8);
}
static void noise(uint8_t env, uint8_t poly) {
  hn = 16;
  NR41_REG = 0; NR42_REG = env; NR43_REG = poly; NR44_REG = 0x80;
}
static void sfx_raise(void) { ch1(0x15, N_C5, 0xA1); }
static void sfx_lower(void) { ch1(0x1D, N_G4, 0xA1); }
static void sfx_deny(void)  { ch1(0x00, N_C4, 0x81); }
static void sfx_mana(void)  { ch1(0x16, N_G5, 0x91); }
static void sfx_flood(void) { noise(0xA3, 0x55); }
static void sfx_food(void)  { ch2(N_E5, 0x71); }
static void sfx_spawn(void) { ch2(N_C6, 0x81); }
static void sfx_fight(void) { noise(0x81, 0x33); }
static void sfx_hit(void)   { noise(0xC2, 0x44); }
static void sfx_qdead(void) { noise(0xF7, 0x77); }
static void jingle(const uint16_t *n, uint8_t len) { tune_p = n; jl = len; ji = 0; jt = 0; }
static void jingle_update(void) {
  if (ji >= jl) return;
  if (jt == 0) { ch2(tune_p[ji++], 0xB2); jt = 10; } else jt--;
}

// ---------- music: 4-channel loop in A minor, 112 BPM, 8 bars (~17 s) ----------
// ch1 = arpeggio, ch2 = lead, ch3 (wave) = bass, ch4 (noise) = drums. SFX borrow channels via h1/h2/hn.
#ifndef WAVERAM
#define WAVERAM ((volatile uint8_t *)0xFF30)
#endif
// pulse-channel frequency register for MIDI notes 36 (C2) .. 99; the wave channel plays one octave lower, so bass uses +12
static const uint16_t NOTE[64] = {
  44, 157, 263, 363, 457, 547, 631, 711,
  786, 856, 923, 986, 1046, 1102, 1155, 1205,
  1253, 1297, 1339, 1379, 1417, 1452, 1486, 1517,
  1547, 1575, 1602, 1627, 1650, 1673, 1694, 1714,
  1732, 1750, 1767, 1783, 1798, 1812, 1825, 1837,
  1849, 1860, 1871, 1881, 1890, 1899, 1907, 1915,
  1923, 1930, 1936, 1943, 1949, 1954, 1959, 1964,
  1969, 1974, 1978, 1982, 1985, 1989, 1992, 1995};
static const uint8_t LEAD[64] = {         // 8 bars x 8 eighth notes, 0 = rest (value = semitones above C2)
  40,0,45,0,43,40,36,0,   41,0,45,0,48,45,41,0,   40,43,40,36,38,40,43,0,   38,0,43,0,47,0,45,43,
  40,0,45,0,48,47,45,40,  41,45,48,0,45,41,40,41, 38,43,47,50,47,43,38,0,    40,44,47,0,47,44,40,0};
// chord roots as semitones above C2
static const uint8_t BROOT[8] = {9, 5, 12, 7, 9, 5, 7, 4};      // Am F C G Am F G E
static const uint8_t BPAT[8]  = {0, 0, 12, 0, 7, 0, 12, 7};     // bass pattern relative to the root
static const uint8_t ARPN[8][3] = {{21,24,28},{17,21,24},{24,28,31},{19,23,26},{21,24,28},{17,21,24},{19,23,26},{16,20,23}};
static const uint8_t ARPO[4]  = {0, 1, 2, 1};
static const uint8_t DPOLY[8] = {0x75,0x21,0x43,0x21,0x75,0x21,0x43,0x21};   // kick hat snare hat kick hat snare hat
static const uint8_t DENV[8]  = {0xA1,0x41,0x81,0x41,0xA1,0x41,0x81,0x41};
static const uint8_t WAVE[16] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10};  // triangle
static uint8_t mus_on, mt, ms;

static void music_start(void) {
  uint8_t i;
  NR30_REG = 0;                                  // DAC off while loading the waveform
  for (i = 0; i < 16; i++) WAVERAM[i] = WAVE[i];
  NR30_REG = 0x80;
  mt = 7; ms = 0; mus_on = 1;                    // first step plays on the next frame
}
static void music_stop(void) {
  mus_on = 0;
  NR12_REG = 0; NR22_REG = 0; NR42_REG = 0; NR30_REG = 0;
}
static void music_update(void) {                 // call once per frame
  uint8_t b, e, n; uint16_t f;
  if (h1) h1--; if (h2) h2--; if (hn) hn--;
  if (!mus_on || ++mt < 8) return;               // 8 frames per 16th note
  mt = 0; b = ms >> 4;
  if (!h1) {                                     // arpeggio on every 16th
    f = NOTE[ARPN[b][ARPO[ms & 3]]];
    NR10_REG = 0; NR11_REG = 0x40; NR12_REG = 0x42; NR13_REG = (uint8_t)f; NR14_REG = 0x80 | (uint8_t)(f >> 8);
  }
  if (!(ms & 1)) {                               // everything else on eighth notes
    e = (ms & 15) >> 1;
    f = NOTE[BROOT[b] + BPAT[e] + 12];           // bass (wave channel, short length for a plucked feel)
    NR30_REG = 0x80; NR31_REG = 0xC8; NR32_REG = 0x40; NR33_REG = (uint8_t)f; NR34_REG = 0xC0 | (uint8_t)(f >> 8);
    n = LEAD[(b << 3) + e];
    if (n && !h2) {                              // lead
      f = NOTE[n];
      NR21_REG = 0x80; NR22_REG = 0x83; NR23_REG = (uint8_t)f; NR24_REG = 0x80 | (uint8_t)(f >> 8);
    }
    if (!hn) { NR41_REG = 0; NR42_REG = DENV[e]; NR43_REG = DPOLY[e]; NR44_REG = 0x80; }   // drums
  }
  ms = (ms + 1) & 127;
}

// ---------- world ----------
static uint8_t is_nest(uint8_t x, uint8_t y) {
  return (x == nestx[0] && y == nesty[0]) || (x == nestx[1] && y == nesty[1]);
}

static void draw_cell(uint8_t x, uint8_t y) {
  uint8_t t = hgt[y][x];
  if (is_nest(x, y)) t = 4; else if (food[y][x] && t) t = 5;
  set_bkg_tile_xy(x, y, BT + t);
}

static uint8_t can_go(uint8_t x, uint8_t y, int8_t dx, int8_t dy) {
  int8_t a = (int8_t)x + dx, b = (int8_t)y + dy, d;
  if (a < 0 || b < 0 || a >= W || b >= H) return 0;
  if (hgt[b][a] == 0) return 0;
  d = (int8_t)hgt[b][a] - (int8_t)hgt[y][x];
  return d >= -1 && d <= 1;           // ants can't climb cliffs: terraform paths!
}

static uint8_t spawn(uint8_t team) {
  uint8_t i, n = 0;
  for (i = 0; i < MAXA; i++) if (ant[i].alive && ant[i].team == team) n++;
  if (n >= MAXA / 2) return 0;                    // each colony gets half the ant slots: fair fight
  for (i = 0; i < MAXA; i++) if (!ant[i].alive) {
    ant[i].x = nestx[team]; ant[i].y = nesty[team];
    ant[i].team = team; ant[i].alive = 1; ant[i].carry = 0;
    ant[i].sol = ((hatched[team]++ & 3) == 0);   // every 4th ant a colony hatches is a soldier
    return 1;
  }
  return 0;
}

static uint8_t dist(uint8_t a, uint8_t b) { return a > b ? a - b : b - a; }

static void count(uint8_t *c) {
  uint8_t i; c[0] = c[1] = 0;
  for (i = 0; i < MAXA; i++) if (ant[i].alive) c[ant[i].team]++;
}

// ---------- Tropico bits: popularity + elections ----------
static void apr(int8_t d) { int16_t v = (int16_t)appr + d; appr = v < 0 ? 0 : v > 99 ? 99 : (uint8_t)v; }
static void die(Ant *a) { a->alive = 0; if (a->team == 0) apr(-2); }     // every dead black ant costs you votes
static void election(void) {
  if (appr >= 50) {                                 // re-elected: foreign aid
    mana = (mana + 8 > MANA_MAX) ? MANA_MAX : mana + 8;
    sfx_mana(); say("ELECTION WON! AID");
  } else if (appr >= 25) {
    sfx_deny(); say("ELECTION: NO BONUS");
  } else {                                          // coup: the rebels loot the treasury
    stock[0] >>= 1; mana = 0; appr = 40;
    sfx_qdead(); say("COUP! COFFERS LOOTED");
  }
}

static void step_ant(Ant *a) {
  uint8_t d, t = a->team, e = t ^ 1, found = 0, bd = 0, tx, ty, de;
  int16_t sc, best = -32000;
  uint8_t soldier = !a->carry && qhp[e] && ncnt[t] >= 8 && a->sol;  // soldiers only march once the colony is big enough
  if (!a->carry && food[a->y][a->x]) { food[a->y][a->x] = 0; a->carry = 1; draw_cell(a->x, a->y); }
  if (a->carry) { uint8_t p = ph[a->y][a->x]; ph[a->y][a->x] = p > 215 ? 255 : p + 40; }
  for (d = 0; d < 4; d++) {
    if (!can_go(a->x, a->y, DX[d], DY[d])) continue;
    tx = a->x + DX[d]; ty = a->y + DY[d];
    if (a->carry) sc = -10 * (int16_t)(dist(tx, nestx[t]) + dist(ty, nesty[t])) + (rand() & 15);
    else if (soldier) {                                 // soldiers march straight for the enemy nest
      de = dist(tx, nestx[e]) + dist(ty, nesty[e]);
      sc = -14 * (int16_t)de + (rand() & 31);
    } else {
      sc = ph[ty][tx] + (food[ty][tx] ? 120 : 0) + (rand() & 31);   // SimAnt pheromone trails
    }
    if (sc > best) { best = sc; bd = d; found = 1; }
  }
  if (found) { a->x += DX[bd]; a->y += DY[bd]; }
  if (a->carry && a->x == nestx[t] && a->y == nesty[t]) {
    a->carry = 0; if (stock[t] < 99) stock[t]++;
    if (t == 0) {
      // mana workflow, step 2: food delivered home is also tribute to the god
      mana = (mana + MANA_PER_FOOD > MANA_MAX) ? MANA_MAX : mana + MANA_PER_FOOD;
      apr(1);                                    // fed ants are happy ants
      sfx_food();
    }
  }
  // at the enemy nest: soldiers bite the queen, other ants steal food and run home
  if (!a->carry && qhp[e] && a->x == nestx[e] && a->y == nesty[e]) {
    if (a->sol) { if (rand() & 1) { qhp[e]--; if (e == 0) apr(-3); if (qhp[e]) sfx_hit(); else sfx_qdead(); } }
    else if (stock[e]) { stock[e]--; a->carry = 1; sfx_fight(); }
  }
}

// Game logic is sliced: each call (one per frame) moves 1/8 of the ants and decays 4 map rows of pheromone;
// every 8th call also resolves fights and runs the colony-level work. Each ant still steps once per 8 frames,
// but the work is spread evenly instead of landing as one big hitch every 8th frame.
static uint8_t slice;
static void tick_slice(void) {
  uint8_t i, j, x, y;
  for (i = slice; i < MAXA; i += 8) {
    if (!ant[i].alive) continue;
    if (hgt[ant[i].y][ant[i].x] == 0) { die(&ant[i]); continue; }            // drowned
    step_ant(&ant[i]);
  }
  if (slice == 7)                                                          // fights: checked once per cycle, as before
    for (i = 0; i < MAXA; i++) if (ant[i].alive) for (j = i + 1; j < MAXA; j++)
      if (ant[j].alive && ant[i].team != ant[j].team && ant[i].x == ant[j].x && ant[i].y == ant[j].y)
        { if (rand() & 1) die(&ant[i]); else die(&ant[j]); sfx_fight(); if (!ant[i].alive) break; }
  if ((tk & 3) == 0) for (y = slice << 2; y < (uint8_t)((slice << 2) + 4); y++) for (x = 0; x < W; x++) if (ph[y][x]) ph[y][x]--;
  if (++slice < 8) return;
  slice = 0;
  tk++;
  count(ncnt);
  for (i = 0; i < 2; i++) {
    if (!qhp[i]) continue;                         // no queen, no eggs
    if (stock[i] >= 3 && spawn(i)) { stock[i] -= 3; if (i == 0) { sfx_spawn(); apr(1); } }
    else if (ncnt[i] == 0 && (tk & 31) == 0) spawn(i);   // emergency egg: never a dead stalemate
    if ((tk & 127) == 0 && qhp[i] < QHP) qhp[i]++; // queens slowly heal
  }
  if ((tk & 31) == 0) apr(!stock[0] && ncnt[0] ? -2 : -1);   // the people are never satisfied; a hungry colony grumbles double
  if (++etk >= 450) { etk = 0; election(); }              // an election roughly every minute
  if ((tk & 7) == 0) for (i = 0; i < 2; i++) {
    x = rand() & (W - 1); y = rand() & (H - 1);
    if (hgt[y][x] && !food[y][x] && !is_nest(x, y)) { food[y][x] = 1; draw_cell(x, y); }
  }
  if ((tk & (MANA_TRICKLE - 1)) == 0 && mana < MANA_MAX) mana++;   // mana workflow, step 1: slow passive trickle
}

static void bump(uint8_t cx0, uint8_t cy0, uint8_t v, uint8_t r) {
  int8_t x, y;
  for (y = (int8_t)cy0 - r; y <= (int8_t)cy0 + r; y++) for (x = (int8_t)cx0 - r; x <= (int8_t)cx0 + r; x++)
    if (x >= 0 && y >= 0 && x < W && y < H) hgt[y][x] = v;
}

// remove accidental cliffs in the generated map (the player makes the real ones)
static void smooth(void) {
  uint8_t pass, x, y, a, b;
  for (pass = 0; pass < 3; pass++) for (y = 0; y < H; y++) for (x = 0; x < W; x++) {
    a = hgt[y][x]; if (!a) continue;
    if (x + 1 < W) { b = hgt[y][x + 1]; if (b) { if (a > b + 1) hgt[y][x] = b + 1; else if (b > a + 1) hgt[y][x + 1] = a + 1; } }
    a = hgt[y][x];
    if (y + 1 < H) { b = hgt[y + 1][x]; if (b) { if (a > b + 1) hgt[y][x] = b + 1; else if (b > a + 1) hgt[y + 1][x] = a + 1; } }
  }
}

static void newgame(void) {
  uint8_t i, k, x, y;
  DISPLAY_OFF;
  for (y = 0; y < H; y++) for (x = 0; x < W; x++) { hgt[y][x] = 1; food[y][x] = 0; ph[y][x] = 0; }
  for (k = 0; k < 28; k++) { x = rand() & (W - 1); y = rand() & (H - 1); bump(x, y, 2, 1); hgt[y][x] = 3; }
  for (k = 0; k < 9; k++) {
    x = rand() & (W - 1); y = rand() & (H - 1);
    hgt[y][x] = 0;
    if (x + 1 < W) hgt[y][x + 1] = 0;
    if (y + 1 < H) hgt[y + 1][x] = 0;
    if (x + 1 < W && y + 1 < H) hgt[y + 1][x + 1] = 0;
  }
  nestx[0] = 4;  nesty[0] = 26; nestx[1] = 27; nesty[1] = 5;
  bump(nestx[0], nesty[0], 1, 1); bump(nestx[1], nesty[1], 1, 1);
  smooth();
  for (k = 0; k < 36; k++) { x = rand() & (W - 1); y = rand() & (H - 1); if (hgt[y][x] && !is_nest(x, y)) food[y][x] = 1; }
  for (i = 0; i < MAXA; i++) ant[i].alive = 0;
  stock[0] = stock[1] = 0; qhp[0] = qhp[1] = QHP; hatched[0] = hatched[1] = 0;
  for (k = 0; k < 3; k++) { spawn(0); spawn(1); }
  count(ncnt);
  ff = 0; appr = 50; etk = 0; slice = 0; hdirty = 1; cheat = 0; cx = nestx[0]; cy = nesty[0] - 2; mana = 10; tk = 0; over = 0;
  camx = 0; camy = H - VH; scx = 0; scy = (int16_t)camy * 8;
  for (y = 0; y < H; y++) for (x = 0; x < W; x++) draw_cell(x, y);
  win_clear(); help_draw(); hud();
  say(sandbox ? "SANDBOX: NO LIMITS" : "A/B LAND START HELP");
  move_win(7, 128);                    // window = 2-row HUD at the bottom
  SCX_REG = (uint8_t)scx; SCY_REG = (uint8_t)scy;
  SHOW_WIN; SHOW_SPRITES;
  DISPLAY_ON;
}

static void raise_land(void) {
  if (mana && hgt[cy][cx] < 3 && !is_nest(cx, cy)) { hgt[cy][cx]++; mana--; draw_cell(cx, cy); sfx_raise(); tev |= 2; return; }
  sfx_deny();
  say(is_nest(cx, cy) ? "NEST CANT BE EDITED" : hgt[cy][cx] >= 3 ? "ALREADY HIGHEST" : "NEED MANA");
}
static void lower_land(void) {
  if (mana && hgt[cy][cx] > 0 && !is_nest(cx, cy)) { hgt[cy][cx]--; mana--; draw_cell(cx, cy); sfx_lower(); tev |= 4; return; }
  sfx_deny();
  say(is_nest(cx, cy) ? "NEST CANT BE EDITED" : hgt[cy][cx] == 0 ? "ALREADY WATER" : "NEED MANA");
}
// mana workflow, step 3: embezzle. Skim colony food (that would hatch ants) into mana, at a cost in popularity.
static void offering(void) {
  if (stock[0] < OFFER_FOOD) { sfx_deny(); say("NEED 2 FOOD"); return; }
  if (mana >= MANA_MAX) { sfx_deny(); say("MANA IS FULL"); return; }
  stock[0] -= OFFER_FOOD;
  mana = (mana + OFFER_MANA > MANA_MAX) ? MANA_MAX : mana + OFFER_MANA;
  apr(-5); sfx_mana(); say("EMBEZZLED! P DOWN");
}
static void flood(void) {
  int8_t x, y;
  if (mana < 8) { sfx_deny(); say("FLOOD NEEDS 8 MANA"); return; }
  mana -= 8; sfx_flood(); tev |= 8;
  for (y = (int8_t)cy - 1; y <= (int8_t)cy + 1; y++) for (x = (int8_t)cx - 1; x <= (int8_t)cx + 1; x++)
    if (x >= 0 && y >= 0 && x < W && y < H && hgt[y][x] && !is_nest(x, y)) { hgt[y][x]--; draw_cell(x, y); }
}

// ---------- camera ----------
static void follow(void) {            // dead-zone camera: scrolls when the cursor nears an edge
  uint8_t t;
  if (cx < camx + 3) camx = cx > 3 ? cx - 3 : 0;
  else if (cx + 3 >= camx + VW) { t = cx + 4 - VW; camx = t > W - VW ? W - VW : t; }
  if (cy < camy + 3) camy = cy > 3 ? cy - 3 : 0;
  else if (cy + 3 >= camy + VH) { t = cy + 4 - VH; camy = t > H - VH ? H - VH : t; }
}
static void scroll_step(void) {       // glide to the target 4px per frame
  int16_t tx = (int16_t)camx * 8, ty = (int16_t)camy * 8;
  if (scx < tx) { scx += 4; if (scx > tx) scx = tx; } else if (scx > tx) { scx -= 4; if (scx < tx) scx = tx; }
  if (scy < ty) { scy += 4; if (scy > ty) scy = ty; } else if (scy > ty) { scy -= 4; if (scy < ty) scy = ty; }
}
static void place(uint8_t s, uint8_t x, uint8_t y) {
  int16_t sx = (int16_t)x * 8 - scx, sy = (int16_t)y * 8 - scy;
  if (sx > -8 && sx < 160 && sy > -8 && sy < 128) move_sprite(s, (uint8_t)(sx + 8), (uint8_t)(sy + 16));
  else move_sprite(s, 0, 0);
}
static void draw_sprites(void) {
  uint8_t i;
  for (i = 0; i < MAXA; i++) {
    if (ant[i].alive) { place(i, ant[i].x, ant[i].y); set_sprite_prop(i, ant[i].team); }
    else move_sprite(i, 0, 0);
  }
  for (i = 0; i < 2; i++) {
    if (qhp[i]) place(MAXA + i, nestx[i], nesty[i]); else move_sprite(MAXA + i, 0, 0);
  }
  place(MAXA + 2, cx, cy);
}

// ---------- screens ----------
static void title(void) {
  uint8_t k;
  DISPLAY_OFF;
  HIDE_SPRITES;
  win_clear();
  title_logo(4, 1);
  put_str(2, 3, "THE RULER OF YOU");
  put_str(1, 4, "VIVA EL PRESIDENTE!");
  put_str(3, 5, "A   RAISE LAND");
  put_str(3, 6, "B   LOWER LAND");
  put_str(3, 7, "SEL FLOOD 3X3");
  put_str(1, 8, "SEL A EMBEZZLE FOOD");
  put_str(1, 9, "FEED YOUR QUEEN:");
  put_str(1, 10, "BLACK ANTS BREED");
  put_str(1, 11, "KILL THE RED QUEEN");
  put_str(1, 12, "TO WIN");
  put_str(1, 13, "HOLD SEL: SANDBOX");
  put_str(1, 14, "START IN GAME: HELP");
  put_str(4, 15, "PRESS START");
  put_str(2, 16, "A: LEARN TO PLAY");
  SCX_REG = 0; SCY_REG = 0;
  move_win(7, 0); SHOW_WIN;
  DISPLAY_ON;
  music_start();
  while (!((k = joypad()) & (J_START | J_A))) { vsync(); seed += DIV_REG + 1; music_update(); }   // seed from how long you wait
  tutor = (k & J_START) ? 0 : 1;       // A alone = guided tutorial
  sandbox = ((k & J_START) && (k & J_SELECT)) ? 1 : 0;   // hold SELECT when pressing START: sandbox (infinite mana, queens can't die)
  logo_cells(4, 1, 1);                 // give the logo cells back to the HUD palette
  waitpadup();
  initrand(seed);
}

// ---------- ANT EYE: first-person 3D view from a black ant (The Sentinel '86 style) ----------
// SELECT+START. 20 columns x 12 window tile rows live in VRAM bank 1 and are redrawn from a ray march over the
// heightmap: checkerboard land, blue water, brown hills, red/black ant posts and tall queen towers.
#define VX 20
#define VR 12
#define VY0 2           // window row of the first view row (HUD above, help text below)
#define VN 24           // ray steps, half a cell each
static const int8_t SIN16[16] = {0, 24, 45, 59, 64, 59, 45, 24, 0, -24, -45, -59, -64, -59, -45, -24};
static const uint8_t BBH[VN + 1] = {0, 40, 32, 21, 16, 12, 10, 9, 8, 7, 6, 5, 5, 4, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2};
// pixel class (0 sky, 1/2 checker land, 3 hill, 4 water, 5 red, 6 black) -> colour of palette 3 / 4 / 5
static const uint8_t PMAP[3][7] = {{0, 1, 2, 3, 0, 0, 0}, {0, 2, 3, 3, 1, 0, 0}, {0, 3, 3, 3, 0, 1, 2}};
static int8_t OFF[7][VN + 1];                     // screen offset of terrain: [height - eye level + 3][depth]
static uint8_t occ[H][W], cls[VR * 8], vang;      // occ: 1 black ant, 2 red ant, 3/4 black/red queen tower

static void view_init(void) {
  uint8_t d, n; int16_t v;
  for (d = 0; d < 7; d++) for (n = 1; n <= VN; n++) {
    v = ((int16_t)(((int8_t)d - 3) * 4 - 3) * 16) / n;
    OFF[d][n] = v > 64 ? 64 : v < -64 ? -64 : (int8_t)v;
  }
}

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
      a = BBH[n]; if (o > 2) a <<= 1;
      a = yt - a;                                    // top row of the post
      e = yt < lb ? yt : lb;
      if (a < 0) a = 0;
      col = (o & 1) ? 6 : 5;
      for (r = a; r < e; r++) cls[r] = col;
      if (a < lim) lim = (uint8_t)a;
    }
  }
}

static void view_render(uint8_t vx, uint8_t vy) {
  uint8_t c, r, k, v, p, m, i, eh = hgt[vy][vx], tl[16];
  for (i = 0; i < MAXA; i++) if (ant[i].alive) occ[ant[i].y][ant[i].x] = ant[i].team + 1;
  for (i = 0; i < 2; i++) if (qhp[i]) occ[nesty[i]][nestx[i]] = 3 + i;
  VBK_REG = VBK_ATTRIBUTES;                          // tile data + attributes both go to bank 1 here
  for (c = 0; c < VX; c++) {
    view_col(c, vx, vy, eh);
    for (r = 0; r < VR; r++) {
      m = 0;
      for (k = 0; k < 8; k++) { v = cls[r * 8 + k]; if (v == 4) m |= 1; else if (v > 4) m |= 2; }
      p = (m & 2) ? 2 : (m & 1) ? 1 : 0;             // palette: ants > water > plain land
      for (k = 0; k < 8; k++) { v = PMAP[p][cls[r * 8 + k]]; tl[k * 2] = (v & 1) ? 0xFF : 0; tl[k * 2 + 1] = (v & 2) ? 0xFF : 0; }
      set_bkg_data(c * VR + r, 1, tl);
      set_win_tile_xy(c, VY0 + r, 8 | (3 + p));      // bit 3 = tile from VRAM bank 1
    }
  }
  VBK_REG = VBK_TILES;
  for (i = 0; i < MAXA; i++) if (ant[i].alive) occ[ant[i].y][ant[i].x] = 0;
  for (i = 0; i < 2; i++) occ[nesty[i]][nestx[i]] = 0;
}

static uint8_t vok(uint8_t i) { return i < MAXA ? (ant[i].alive && ant[i].team == 0) : qhp[0] > 0; }

static void eye(void) {                              // time stands still while you look through a black ant's eyes
  uint8_t i, x, y, k, p, prev, vi = MAXA, best = 255, d, rep = 0, go = 1, vx = 0, vy = 0;
  tev |= 16;
  for (i = 0; i < MAXA; i++) if (vok(i)) { d = dist(ant[i].x, cx) + dist(ant[i].y, cy); if (d < best) { best = d; vi = i; } }
  HIDE_SPRITES;
  VBK_REG = VBK_TILES;
  for (x = 0; x < VX; x++) for (y = 0; y < VR; y++) set_win_tile_xy(x, VY0 + y, x * VR + y);
  for (y = 14; y < 18; y++) for (x = 0; x < 20; x++) set_win_tile_xy(x, y, FT);
  put_str(0, 14, "A:NEXT ANT B:GO HERE");
  put_str(0, 15, "L/R TURN START BACK");
  put_str(1, 16, "TIME STANDS STILL");
  prev = joypad();
  while (1) {
    if (go) {
      if (vi < MAXA) { vx = ant[vi].x; vy = ant[vi].y; } else { vx = nestx[0]; vy = nesty[0]; }
      view_render(vx, vy); move_win(7, 0); go = 0;
    }
    vsync(); music_update();
    k = joypad(); p = k & ~prev; prev = k;
    if (p & J_START) break;
    if (p & J_B) { cx = vx; cy = vy; break; }          // transfer: the cursor jumps to where you were looking from
    if (p & J_A) { i = 0; do { vi = (vi == MAXA) ? 0 : vi + 1; } while (!vok(vi) && ++i <= MAXA); sfx_food(); go = 1; }
    if (k & (J_LEFT | J_RIGHT)) {
      if (rep == 0 || (rep >= 8 && !(rep & 3))) { vang = (vang + ((k & J_RIGHT) ? 1 : 15)) & 15; go = 1; }
      if (rep < 250) rep++;
    } else rep = 0;
  }
  VBK_REG = VBK_ATTRIBUTES;                          // give the view cells back to the HUD palette
  for (x = 0; x < VX; x++) for (y = 0; y < VR; y++) set_win_tile_xy(x, VY0 + y, 1);
  VBK_REG = VBK_TILES;
  win_clear(); help_draw(); hdirty = 1;
  move_win(7, 128); SHOW_SPRITES;
  waitpadup();
}

// ---------- TUTORIAL: interactive lessons (title screen: press A) ----------
// Each lesson is a full-screen card (START next, B skip), then - if it has a task - a live hint on the HUD until you do it.
#define TN 10
static const uint8_t TEV[TN] = {0, 1, 2, 4, 0, 8, 16, 0, 0, 0};     // event bit that completes each lesson (0 = read only)
static const char *const THINT[TN] = {0, "TRY: MOVE THE CURSOR", "TRY: PRESS A: RAISE", "TRY: PRESS B: LOWER", 0,
  "TAP SELECT TO FLOOD", "SEL START: ANT EYE", 0, 0, 0};
static const char *const TCARD[TN][12] = {
 {"WELCOME PRESIDENT!", "YOU RULE THE BLACK", "ANTS OF EMPIRE ANTS", "YOU CANT GIVE THEM", "ORDERS: INSTEAD YOU", "SHAPE THE LAND AND",
  "THEY WALK AROUND IT", "", "GOAL: KILL THE RED", "QUEEN BEFORE THEY", "KILL YOURS"},
 {"1/8 THE CURSOR", "THE YELLOW FRAME IS", "YOUR CURSOR: LAND", "TOOLS WORK ON THE", "TILE UNDER IT", "", "D PAD MOVES IT",
  "HOLD TO REPEAT", "THE MAP SCROLLS NEAR", "THE EDGE", "", "NOW TRY IT!"},
 {"2/8 RAISE LAND", "LAND HAS 4 HEIGHTS:", "0 WATER   1 SAND", "2 GRASS   3 HILL", "", "A RAISES THE TILE", "UNDER THE CURSOR",
  "COST: 1 MANA", "ANTS CANT CLIMB MORE", "THAN 1 STEP: BUILD", "RAMPS AND STAIRS!", "NESTS CANT BE EDITED"},
 {"3/8 LOWER LAND", "B LOWERS THE TILE", "COST: 1 MANA", "", "LEVEL 0 IS WATER:", "ANTS CANT WALK ON IT", "AND DROWN IF FLOODED",
  "DIG MOATS TO STOP", "RED ANTS: RAISE LAND", "TO BRIDGE GAPS"},
 {"4/8 MANA", "MP IS YOUR MANA:", "EVERY EDIT COSTS MP", "", "MP COMES FROM:", " SLOW TRICKLE", " FOOD CARRIED HOME",
  " ELECTION AID", "", "SEL A EMBEZZLES 2", "FOOD INTO 4 MP BUT", "P DROPS BY 5"},
 {"5/8 FLOOD", "TAP SELECT ALONE:", "LOWERS A 3X3 AREA BY", "ONE LEVEL: COST 8 MP", "", "ANTS ON TILES THAT", "HIT LEVEL 0 DROWN:",
  "GREAT AGAINST RED", "ARMIES BUT CAREFUL", "WITH YOUR OWN!", "MP REFILLED FOR YOU"},
 {"6/8 ANT EYE", "SEE THE WORLD LIKE", "A BLACK ANT:", "HOLD SELECT AND TAP", "START", "L R  TURN", "A    NEXT ANT",
  "B    JUMP CURSOR TO", "     THIS SPOT", "START  BACK", "RED POSTS: ENEMIES", "TALL: QUEENS"},
 {"7/8 THE COLONY", "BLACK ANTS FIND FOOD", "AND CARRY IT HOME", "3 FOOD HATCHES A", "NEW ANT", "EVERY 4TH IS A",
  "SOLDIER: WITH 8 ANTS", "THEY MARCH ON THE", "RED NEST", "ANTS FOLLOW TRAILS:", "MAKE EASY PATHS!"},
 {"8/8 POPULARITY", "P IS POPULARITY", "FOOD AND NEW ANTS", "RAISE P: DEAD ANTS", "AND HUNGER CUT IT", "",
  "ELECTION EVERY MIN:", "P 50 UP: 8 MP AID", "P UNDER 25: COUP!", "COFFERS LOOTED"},
 {"READY TO RULE!", "KILL THE RED QUEEN", "TO WIN: LOSE YOURS", "AND ITS OVER", "", "START: PAUSE HELP",
  "SEL B: FAST FORWARD", "", "VIVA EL PRESIDENTE!", "GOOD LUCK!"}};

static void tut_card(uint8_t i) {
  uint8_t r, k, prev;
  win_clear();
  put_str(0, 1, TCARD[i][0]);
  for (r = 1; r < 12 && TCARD[i][r]; r++) put_str(0, 2 + r, TCARD[i][r]);
  put_str(0, 16, "START: NEXT  B: SKIP");
  HIDE_SPRITES; move_win(7, 0);
  prev = joypad();
  while (1) {
    vsync(); music_update();
    k = joypad();
    if ((k & ~prev) & J_START) break;
    if ((k & ~prev) & J_B) { tut = 0; break; }
    prev = k;
  }
  win_clear(); help_draw(); hdirty = 1;
  move_win(7, 128); SHOW_SPRITES; waitpadup();
}
static void tut_enter(void) {                          // show lessons until one needs the player to do something
  while (tut) {
    if (tut > TN) { tut = 0; say("GOOD LUCK PRESIDENT!"); sfx_mana(); break; }
    tut_card(tut - 1);
    if (!tut) { say("TUTORIAL SKIPPED"); break; }
    tev = 0; lcx = cx; lcy = cy;
    if (mana < 10) mana = 10;                          // always enough MP to practise
    if (tut == 6) mana = MANA_MAX;                     // flood costs 8
    if (TEV[tut - 1]) break;
    tut++;
  }
}

static void play(void) {
  uint8_t k, prev = 0, p, dirs, last = 0, rep = 0, fire, t = 4, ki = 0, paused = 0, selused = 0, selprev = 0, n;
  if (tutor) { tut = 1; tut_enter(); }
  while (!over) {
    vsync();
    music_update();
    SCX_REG = (uint8_t)scx; SCY_REG = (uint8_t)scy;
    k = joypad(); p = k & ~prev; prev = k;
    if ((p & J_START) && (k & J_SELECT) && !paused) { selused = 1; eye(); continue; }   // SELECT+START: ant eye
    if (p & J_START) {                 // START: pause + help screen
      paused ^= 1;
      if (paused) { move_win(7, 0); HIDE_SPRITES; } else { move_win(7, 128); SHOW_SPRITES; }
    }
    if (paused) continue;
    dirs = k & (J_LEFT | J_RIGHT | J_UP | J_DOWN);
    if (dirs != last) { rep = 0; last = dirs; }
    if (dirs) {                        // hold to repeat after a short delay
      fire = (rep == 0) || (rep >= 10 && (rep % 3) == 1);
      if (rep < 250) rep++;
      if (fire) {
        if ((dirs & J_LEFT) && cx > 0) cx--;
        if ((dirs & J_RIGHT) && cx < W - 1) cx++;
        if ((dirs & J_UP) && cy > 0) cy--;
        if ((dirs & J_DOWN) && cy < H - 1) cy++;
      }
    }
    if (p) {                           // Konami code tracker
      if (p == KONAMI[ki]) { if (++ki == 10) { ki = 0; cheat = 1; sfx_mana(); say("CHEAT ON! INFINITE"); } }
      else ki = (p == KONAMI[0]) ? 1 : 0;
    }
    if (k & J_SELECT) {               // SELECT is a modifier: SEL+A embezzle, SEL+B fast forward, alone (on release) flood
      if (p & J_SELECT) selused = 0;
      if (p & J_A) { offering(); selused = 1; }
      if (p & J_B) { ff ^= 1; say(ff ? "FAST FORWARD ON" : "FAST FORWARD OFF"); selused = 1; }
    } else {
      if (selprev && !selused) flood();
      if (p & J_A) raise_land();
      if (p & J_B) lower_land();
    }
    selprev = k & J_SELECT;
    if (tut && (cx != lcx || cy != lcy)) { tev |= 1; lcx = cx; lcy = cy; }
    if (tut && (tev & TEV[tut - 1])) { sfx_mana(); tut++; tut_enter(); }     // task done: next lesson
    if (cheat) { mana = MANA_MAX; stock[0] = 99; appr = 99; }
    if (sandbox) { mana = MANA_MAX; qhp[0] = qhp[1] = QHP; appr = 99; }
    if (tut) { qhp[0] = qhp[1] = QHP; if (appr < 50) appr = 50; }          // no game over mid-lesson
    follow(); scroll_step();
    for (n = ff ? 4 : 1; n; n--) tick_slice();     // fast forward = 4 slices per frame
    if (++t >= 8) {
      t = 0; count(ncnt);
      if (tut && !msgt && THINT[tut - 1]) { msg = THINT[tut - 1]; msgt = 1; }   // keep the lesson goal on the HUD
      hud();
    }
    if (!qhp[1]) over = 1; else if (!qhp[0]) over = 2;
    draw_sprites();
  }
  music_stop();
  put_str(0, 1, over == 1 ? "YOU WIN! PRESS START" : "COLONY LOST! START  ");
  if (over == 1) jingle(WIN_TUNE, 6); else jingle(LOSE_TUNE, 4);
  while (1) { vsync(); jingle_update(); if (joypad() & J_START) break; }
  waitpadup();
}

void main(void) {
  uint8_t i, x, y;
  uint8_t buf[40 * 16], sp[3 * 16];
  dippinn_logo_play();                                   // DippInn boot logo (~8 s, START skips)
  NR52_REG = 0x80; NR51_REG = 0xFF; NR50_REG = 0x77;     // sound on, all channels, full volume
  DISPLAY_OFF;
  for (i = 0; i < 6; i++) mk(buf + i * 16, BG[i]);
  set_bkg_data(BT, 6, buf);
  for (i = 0; i < 2; i++) mk(buf + i * 16, BAR[i]);
  set_bkg_data(BAR_F, 2, buf);
  for (i = 0; i < 40; i++) mk_glyph(buf + i * 16, FONT[i]);
  set_bkg_data(FT, 40, buf);
  title_tiles(buf);
  set_bkg_data(TT, 13, buf);
  for (i = 0; i < 3; i++) mk(sp + i * 16, SP[i]);
  set_sprite_data(0, 3, sp);
  set_bkg_palette(0, 6, bpal);
  view_init();
  set_sprite_palette(0, 3, spal);
  VBK_REG = VBK_ATTRIBUTES;                              // BG layer: palette 0, window: palette 1
  for (y = 0; y < 32; y++) for (x = 0; x < 32; x++) set_bkg_tile_xy(x, y, 0);
  for (y = 0; y < 18; y++) for (x = 0; x < 20; x++) set_win_tile_xy(x, y, 1);
  VBK_REG = VBK_TILES;
  LCDC_REG = (uint8_t)((LCDC_REG | LCDCF_WIN9C00) & (uint8_t)~LCDCF_BG9C00); // window map 9C00, world map 9800
  SPRITES_8x8;
  for (i = 0; i < MAXA; i++) set_sprite_tile(i, 0);
  set_sprite_tile(MAXA, 2);     set_sprite_prop(MAXA, 0);       // black queen
  set_sprite_tile(MAXA + 1, 2); set_sprite_prop(MAXA + 1, 1);   // red queen
  set_sprite_tile(MAXA + 2, 1); set_sprite_prop(MAXA + 2, 2);   // cursor
  SHOW_BKG;
  while (1) { title(); newgame(); play(); }
}
