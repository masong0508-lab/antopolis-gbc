// ANTOPOLIS - Populous x SimAnt for Game Boy Color (GBDK-2020)
// 32x32 scrolling world, queen ants, sound. All graphics generated in code.
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <rand.h>

#define W 32            // world size = size of the hardware BG map
#define H 32
#define VW 20           // visible world tiles (bottom 2 rows are the HUD window)
#define VH 16
#define MAXA 34         // workers (queens + cursor sprites come after these)
#define QHP 5
#define MANA_MAX 20     // mana cap
#define MANA_TRICKLE 32 // passive: +1 mana every this many ticks
#define MANA_PER_FOOD 1 // tribute: mana per food a black ant carries home
#define OFFER_FOOD 2    // offering (START): food spent ...
#define OFFER_MANA 4    // ... for this much mana
#define BT 128          // first terrain tile
#define FT 136          // first font tile (40 glyphs)

typedef struct { uint8_t x, y, team, alive, carry, sol; } Ant;

static uint8_t hgt[H][W], food[H][W], ph[H][W];
static Ant ant[MAXA];
static uint8_t nestx[2], nesty[2], stock[2], qhp[2], ncnt[2], hatched[2];
static uint8_t cx, cy, mana, tk, over, camx, camy, sandbox, cheat;
static const char *msg; static uint8_t msgt;      // short HUD hint (replaces the QUEEN row for ~1.5 s)
static int16_t scx, scy;                       // pixel scroll
static uint16_t seed;
// Konami code (in play): infinite mana + food. Ends on A, not START (START = offering).
static const uint8_t KONAMI[10] = {J_UP, J_UP, J_DOWN, J_DOWN, J_LEFT, J_RIGHT, J_LEFT, J_RIGHT, J_B, J_A};
static const int8_t DX[4] = {1, -1, 0, 0};
static const int8_t DY[4] = {0, 0, 1, -1};

// ---------- palettes & graphics (generated from strings) ----------
static const palette_color_t bpal[8] = {
  RGB(8,16,28), RGB(28,24,14), RGB(8,20,6), RGB(6,4,3),      // 0: terrain
  RGB(0,0,0),   RGB(8,8,8),    RGB(20,20,20), RGB(31,31,31)};// 1: HUD text
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
static void win_clear(void) {
  uint8_t x, y;
  for (y = 0; y < 18; y++) for (x = 0; x < 20; x++) set_win_tile_xy(x, y, FT);
}
static void say(const char *m) { msg = m; msgt = 12; }
static void hud(void) {
  put_str(0, 0, "MP:");     put_num(3, 0, mana);
  put_str(5, 0, " ANT:");   put_num(10, 0, ncnt[0]);
  put_str(12, 0, " RED:");  put_num(17, 0, ncnt[1]);
  if (msgt) {                                   // hint overlay: text padded to the full 20 columns
    uint8_t x = 0; const char *m = msg;
    while (x < 20) put_char(x++, 1, *m ? *m++ : ' ');
    msgt--;
    return;
  }
  put_str(0, 1, "QUEEN:");  put_char(6, 1, '0' + qhp[0]);
  put_str(7, 1, " FOE:");   put_char(12, 1, '0' + qhp[1]);
  put_str(13, 1, " F:");    put_num(16, 1, stock[0]);
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

static void ch1(uint8_t sweep, uint16_t f, uint8_t env) {
  NR10_REG = sweep; NR11_REG = 0x80; NR12_REG = env;
  NR13_REG = (uint8_t)f; NR14_REG = 0x80 | (uint8_t)(f >> 8);
}
static void ch2(uint16_t f, uint8_t env) {
  NR21_REG = 0x80; NR22_REG = env;
  NR23_REG = (uint8_t)f; NR24_REG = 0x80 | (uint8_t)(f >> 8);
}
static void noise(uint8_t env, uint8_t poly) {
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
      sfx_food();
    }
  }
  // at the enemy nest: soldiers bite the queen, other ants steal food and run home
  if (!a->carry && qhp[e] && a->x == nestx[e] && a->y == nesty[e]) {
    if (a->sol) { if (rand() & 1) { qhp[e]--; if (qhp[e]) sfx_hit(); else sfx_qdead(); } }
    else if (stock[e]) { stock[e]--; a->carry = 1; sfx_fight(); }
  }
}

static void tick(void) {
  uint8_t i, j, x, y;
  tk++;
  count(ncnt);
  for (i = 0; i < MAXA; i++) if (ant[i].alive && hgt[ant[i].y][ant[i].x] == 0) ant[i].alive = 0;  // drowned
  for (i = 0; i < MAXA; i++) if (ant[i].alive) step_ant(&ant[i]);
  for (i = 0; i < MAXA; i++) for (j = i + 1; j < MAXA; j++)
    if (ant[i].alive && ant[j].alive && ant[i].team != ant[j].team && ant[i].x == ant[j].x && ant[i].y == ant[j].y)
      { if (rand() & 1) ant[i].alive = 0; else ant[j].alive = 0; sfx_fight(); }
  for (i = 0; i < 2; i++) {
    if (!qhp[i]) continue;                         // no queen, no eggs
    if (stock[i] >= 3 && spawn(i)) { stock[i] -= 3; if (i == 0) sfx_spawn(); }
    else if (ncnt[i] == 0 && (tk & 31) == 0) spawn(i);   // emergency egg: never a dead stalemate
    if ((tk & 127) == 0 && qhp[i] < QHP) qhp[i]++; // queens slowly heal
  }
  if ((tk & 3) == 0) for (y = 0; y < H; y++) for (x = 0; x < W; x++) if (ph[y][x]) ph[y][x]--;
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
  cheat = 0; cx = nestx[0]; cy = nesty[0] - 2; mana = 10; tk = 0; over = 0;
  camx = 0; camy = H - VH; scx = 0; scy = (int16_t)camy * 8;
  for (y = 0; y < H; y++) for (x = 0; x < W; x++) draw_cell(x, y);
  win_clear(); hud();
  say(sandbox ? "SANDBOX: NO LIMITS" : "A/B LAND SEL FLOOD");
  move_win(7, 128);                    // window = 2-row HUD at the bottom
  SCX_REG = (uint8_t)scx; SCY_REG = (uint8_t)scy;
  SHOW_WIN; SHOW_SPRITES;
  DISPLAY_ON;
}

static void raise_land(void) {
  if (mana && hgt[cy][cx] < 3 && !is_nest(cx, cy)) { hgt[cy][cx]++; mana--; draw_cell(cx, cy); sfx_raise(); return; }
  sfx_deny();
  say(is_nest(cx, cy) ? "NEST CANT BE EDITED" : hgt[cy][cx] >= 3 ? "ALREADY HIGHEST" : "NEED MANA");
}
static void lower_land(void) {
  if (mana && hgt[cy][cx] > 0 && !is_nest(cx, cy)) { hgt[cy][cx]--; mana--; draw_cell(cx, cy); sfx_lower(); return; }
  sfx_deny();
  say(is_nest(cx, cy) ? "NEST CANT BE EDITED" : hgt[cy][cx] == 0 ? "ALREADY WATER" : "NEED MANA");
}
// mana workflow, step 3: offering. Trade colony food (that would hatch ants) for a burst of mana.
static void offering(void) {
  if (stock[0] < OFFER_FOOD) { sfx_deny(); say("NEED 2 FOOD"); return; }
  if (mana >= MANA_MAX) { sfx_deny(); say("MANA IS FULL"); return; }
  stock[0] -= OFFER_FOOD;
  mana = (mana + OFFER_MANA > MANA_MAX) ? MANA_MAX : mana + OFFER_MANA;
  sfx_mana();
}
static void flood(void) {
  int8_t x, y;
  if (mana < 8) { sfx_deny(); say("FLOOD NEEDS 8 MANA"); return; }
  mana -= 8; sfx_flood();
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
  put_str(5, 1, "ANTOPOLIS");
  put_str(2, 3, "GOD OF THE ANTS");
  put_str(3, 5, "A   RAISE LAND");
  put_str(3, 6, "B   LOWER LAND");
  put_str(3, 7, "SEL FLOOD 3X3");
  put_str(3, 8, "STA FOOD TO MANA");
  put_str(1, 9, "FEED YOUR QUEEN:");
  put_str(1, 10, "BLACK ANTS BREED");
  put_str(1, 11, "KILL THE RED QUEEN");
  put_str(1, 12, "TO WIN");
  put_str(1, 13, "HOLD SEL: SANDBOX");
  put_str(4, 15, "PRESS START");
  SCX_REG = 0; SCY_REG = 0;
  move_win(7, 0); SHOW_WIN;
  DISPLAY_ON;
  while (!((k = joypad()) & J_START)) { vsync(); seed += DIV_REG + 1; }   // seed from how long you wait
  sandbox = (k & J_SELECT) ? 1 : 0;   // hold SELECT when pressing START: sandbox (infinite mana, queens can't die)
  waitpadup();
  initrand(seed);
}

static void play(void) {
  uint8_t k, prev = 0, p, dirs, last = 0, rep = 0, fire, t = 0, ki = 0;
  while (!over) {
    vsync();
    SCX_REG = (uint8_t)scx; SCY_REG = (uint8_t)scy;
    k = joypad(); p = k & ~prev; prev = k;
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
    if (p & J_A) raise_land();
    if (p & J_B) lower_land();
    if (p & J_SELECT) flood();
    if (p & J_START) offering();
    if (cheat) { mana = MANA_MAX; stock[0] = 99; }
    if (sandbox) { mana = MANA_MAX; qhp[0] = qhp[1] = QHP; }
    follow(); scroll_step();
    if (++t >= 8) {
      t = 0; tick(); count(ncnt); hud();
      if (!qhp[1]) over = 1; else if (!qhp[0]) over = 2;
    }
    draw_sprites();
  }
  put_str(0, 1, over == 1 ? "YOU WIN! PRESS START" : "COLONY LOST! START  ");
  if (over == 1) jingle(WIN_TUNE, 6); else jingle(LOSE_TUNE, 4);
  while (1) { vsync(); jingle_update(); if (joypad() & J_START) break; }
  waitpadup();
}

void main(void) {
  uint8_t i, x, y;
  uint8_t buf[40 * 16], sp[3 * 16];
  NR52_REG = 0x80; NR51_REG = 0xFF; NR50_REG = 0x77;     // sound on, all channels, full volume
  DISPLAY_OFF;
  for (i = 0; i < 6; i++) mk(buf + i * 16, BG[i]);
  set_bkg_data(BT, 6, buf);
  for (i = 0; i < 40; i++) mk_glyph(buf + i * 16, FONT[i]);
  set_bkg_data(FT, 40, buf);
  for (i = 0; i < 3; i++) mk(sp + i * 16, SP[i]);
  set_sprite_data(0, 3, sp);
  set_bkg_palette(0, 2, bpal);
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
