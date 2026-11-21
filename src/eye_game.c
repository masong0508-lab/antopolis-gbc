// Real ANT EYE gameplay in switchable ROM bank 1, leaving fixed bank 0 for the colony simulation.
#pragma bank 1
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>
#define W 32
#define H 32
#define MAXA 50
#define VX 20
#define VR 12
#define VY0 2
#define FT 136
#define MANA_MAX 20
#define MANA_PER_FOOD 1
typedef struct { uint8_t x, y, team, alive, carry, sol; } Ant;
extern Ant ant[MAXA];
extern uint8_t food[H][W], ph[H][W], occ[H][W], vang;
extern uint8_t nestx[2], nesty[2], stock[2], qhp[2];
extern uint8_t cx, cy, mana, over, tev, hdirty;
extern const int8_t SIN16[16];
extern uint8_t vok(uint8_t);
extern uint8_t dist(uint8_t, uint8_t);
extern void eye_step(uint8_t *, uint8_t *, int8_t);
extern void eye_status(uint8_t, uint8_t);
extern void occ_set(uint8_t);
extern void put_str(uint8_t, uint8_t, const char *);
extern void win_clear(void);
extern void draw_cell(uint8_t, uint8_t);
extern void apr(int8_t);
extern void die(Ant *);
extern void sfx_food(void);
extern void sfx_mana(void);
extern void sfx_deny(void);
extern void sfx_fight(void);
extern void sfx_hit(void);
extern void view_cols(uint8_t, uint8_t, uint8_t, uint8_t) BANKED;

// Pilot the selected worker in the real map; first-person movement is no longer a camera-only trick.
// Keep the colony simulation paused to leave enough GBC time for ray rendering, but preserve all changes.
static void eye_pilot(uint8_t vi, uint8_t vx, uint8_t vy, uint8_t scent) {
  Ant *a;
  if (vi >= MAXA || !vok(vi)) return;
  a = &ant[vi]; a->x = vx; a->y = vy;
  if (scent) ph[vy][vx] = 255;
  if (!a->carry && food[vy][vx]) {
    food[vy][vx] = 0; a->carry = 1; draw_cell(vx, vy); sfx_food();
  }
  if (a->carry && vx == nestx[0] && vy == nesty[0]) {
    a->carry = 0;
    if (stock[0] < 99) stock[0]++;
    mana = (mana + MANA_PER_FOOD > MANA_MAX) ? MANA_MAX : mana + MANA_PER_FOOD;
    apr(1); sfx_mana();
  }
  memset(occ, 0, sizeof occ); occ_set(1);
}

// A bite has an actual target in the cell ahead; this affects ants and the enemy queen in the real game.
static void eye_bite(uint8_t vi, uint8_t vx, uint8_t vy) {
  int8_t fx, fy, dx = 0, dy = 0;
  int16_t tx, ty; uint8_t i;
  if (vi >= MAXA || !vok(vi)) return;
  fx = SIN16[(vang + 4) & 15]; fy = SIN16[vang];
  if ((fx < 0 ? -fx : fx) >= (fy < 0 ? -fy : fy)) dx = fx < 0 ? -1 : 1;
  else dy = fy < 0 ? -1 : 1;
  tx = (int16_t)vx + dx; ty = (int16_t)vy + dy;
  if (tx < 0 || ty < 0 || tx >= W || ty >= H) { sfx_deny(); return; }
  for (i = 0; i < MAXA; i++) if (ant[i].alive && ant[i].team == 1 && ant[i].x == tx && ant[i].y == ty) {
    die(&ant[i]); sfx_fight(); memset(occ, 0, sizeof occ); occ_set(1); return;
  }
  if (tx == nestx[1] && ty == nesty[1] && qhp[1]) {
    qhp[1]--; sfx_hit(); if (!qhp[1]) over = 1;
    memset(occ, 0, sizeof occ); occ_set(1); return;
  }
  sfx_deny();
}

void eye(void) BANKED {                              // pilot a black ant; food, scents and combat persist in the colony
  uint8_t i, x, y, k, p, prev, vi = MAXA, best = 255, d, rep = 0, go = 1, vx = 0, vy = 0, vcol = VX, dirs, last = 0;
  tev |= 16;
  for (i = 0; i < MAXA; i++) if (vok(i)) { d = dist(ant[i].x, cx) + dist(ant[i].y, cy); if (d < best) { best = d; vi = i; } }
  HIDE_SPRITES;
  VBK_REG = VBK_TILES;
  for (x = 0; x < VX; x++) for (y = 0; y < VR; y++) set_win_tile_xy(x, VY0 + y, x * VR + y);
  for (y = 14; y < 18; y++) for (x = 0; x < 20; x++) set_win_tile_xy(x, y, FT);
  put_str(0, 14, "A:NEXT ANT B:BITE   ");
  put_str(0, 15, "L/R TURN U/D WALK   ");
  put_str(0, 16, "SEL:SCENT START:MAP ");
  occ_set(1);                                        // ants cannot move in here: mark them once
  move_win(7, 0);
  prev = joypad();
  while (1) {
    if (go) {                                        // 1 = jump to the chosen ant, 2 = same spot (turned / walked)
      if (go == 1) { if (vi < MAXA) { vx = ant[vi].x; vy = ant[vi].y; } else { vx = nestx[0]; vy = nesty[0]; } }
      go = 0; vcol = 0; eye_status(vx, vy);
      put_str(0, 14, vi < MAXA && ant[vi].carry ? "FOOD: TAKE HOME!    " : "A:NEXT ANT B:BITE   ");
    }
    vsync();
    if (vcol < VX) { view_cols(vx, vy, vcol, vcol + 4); vcol += 4; }   // 4 columns per frame: the view sweeps in, input stays live
    k = joypad(); p = k & ~prev; prev = k;
    if (p & J_START) break;
    if (p & J_B) { eye_bite(vi, vx, vy); go = 2; if (over) break; }
    if (p & J_A) { i = 0; do { vi = (vi == MAXA) ? 0 : vi + 1; } while (!vok(vi) && ++i <= MAXA); sfx_food(); go = 1; }
    if ((k & J_SELECT) && vi < MAXA) ph[vy][vx] = 255;
    dirs = k & (J_LEFT | J_RIGHT | J_UP | J_DOWN);
    if (dirs != last) { rep = 0; last = dirs; }
    if (dirs) {
      if (rep == 0 || (rep >= 8 && !(rep & 3))) {
        if (dirs & J_RIGHT) { vang = (vang + 1) & 15; go = 2; }
        else if (dirs & J_LEFT) { vang = (vang + 15) & 15; go = 2; }
        else {
          x = vx; y = vy; eye_step(&vx, &vy, (dirs & J_UP) ? 1 : -1);
          if (vx != x || vy != y) { eye_pilot(vi, vx, vy, (uint8_t)(k & J_SELECT)); go = 2; }
          else sfx_deny();
        }
      }
      if (rep < 250) rep++;
    }
  }
  cx = vx; cy = vy;                       // map cursor follows the actual pilot when leaving ANT EYE
  memset(occ, 0, sizeof occ);
  win_clear(); hdirty = 1;
  move_win(7, 128); SHOW_SPRITES;
  waitpadup();
}

