#!/usr/bin/env python3
"""Exercise actual GBC soldier movement and protect the one-scan cadence."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src/main.c").read_text()
makefile = (ROOT / "Makefile").read_text()
assert '#include "colony_defense.h"' in source
assert "static uint8_t defend_i[2];" in source
step = source.split("static void step_ant(Ant *a) {", 1)[1].split(
    "// Game logic is sliced:", 1
)[0]
tick = source.split("static void tick_slice(void) {", 1)[1].split(
    "// One-time terrain shaping", 1
)[0]
newgame = source.split("static void newgame(uint8_t load) {", 1)[1].split(
    "static void swp(void)", 1
)[0]
assert "colony_guard_intercept(" in step
assert "goal_x = enemy->x; goal_y = enemy->y; soldier = 1;" in step
assert "sc = -14 * (int16_t)de + (rand() & 31);" in step
assert "colony_home_threats(" not in step
assert tick.count("colony_home_threats(") == 1
assert tick.index("if (++slice < 8) return;") < tick.index("colony_home_threats(")
assert "defend_i[0] = defend_i[1] = MAXA;" in newgame
assert "tools/test_colony_defense.c" in makefile

FIXTURE = r"""
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "colony_defense.h"
#include "colony_route.h"
typedef EyeAnt Ant;
static Ant ant[EYE_OCC_ANTS];
static uint8_t nestx[2], nesty[2], qhp[2], ncnt[2], thr[2];
static uint8_t defend_i[2], stock[2], ph[32][32], food[32][32];
static uint8_t pl,mp,mana,omana,sandbox,tut;
#define MANA_PER_FOOD 2
#define MANA_MAX 99
static unsigned rng_calls;
static const int8_t DX[4]={1,-1,0,0}, DY[4]={0,0,1,-1};
static uint8_t dist(uint8_t a,uint8_t b) { return a>b?a-b:b-a; }
static uint8_t can_go(uint8_t x,uint8_t y,int8_t dx,int8_t dy) {
  int xx=(int)x+dx, yy=(int)y+dy;
  return (uint8_t)(xx>=0 && xx<32 && yy>=0 && yy<32);
}
static unsigned fake_rand(void) { ++rng_calls; return 0; }
#define rand fake_rand
static void draw_cell(uint8_t x,uint8_t y) { (void)x; (void)y; }
static void sfx_food(void) {}
static void sfx_qdead(void) {}
static void sfx_hit(void) {}
static void sfx_fight(void) {}
static void apr(int amount) { (void)amount; }
"""
TESTS = r"""
static void setup(uint8_t t) {
  uint8_t e=t^1u;
  memset(ant,0,sizeof ant);
  memset(ph,0,sizeof ph);
  memset(food,0,sizeof food);
  memset(stock,0,sizeof stock);
  nestx[t]=10; nesty[t]=10; nestx[e]=2; nesty[e]=10;
  qhp[0]=qhp[1]=10; ncnt[0]=ncnt[1]=3; thr[0]=thr[1]=12;
  ant[0]=(Ant){10,10,t,1,0,1};
  ant[1]=(Ant){11,10,e,1,0,0};
  colony_home_threats(ant,nestx,nesty,defend_i);
  assert(defend_i[t]==1);
  assert(eye_occupancy_at(10,10,ant,food,qhp,nestx,nesty)==3u+t);
  rng_calls=0;
}
static void check_team(uint8_t t) {
  setup(t);
  // Adjacent food and scent on the opposite side lure a forager away,
  // but even a three-ant young colony sends an unloaded soldier east.
  food[10][9]=1;
  step_ant(&ant[0]);
  assert(ant[0].x==11 && ant[0].y==10 && rng_calls==4);
  // Normal workers still forage instead of being drafted.
  setup(t); ant[0].sol=0; food[10][9]=1;
  step_ant(&ant[0]); assert(ant[0].x==9 && rng_calls==4);
  // A cached enemy that leaves the five-tile radius must not trigger
  // pursuit. A young soldier resumes its ordinary food gathering.
  setup(t); ant[1].x=16; food[10][9]=1;
  step_ant(&ant[0]); assert(ant[0].x==9 && rng_calls==4);
  // A deleted/reused enemy slot must be ignored.
  setup(t); ant[1].alive=0; food[10][9]=1;
  step_ant(&ant[0]); assert(ant[0].x==9 && rng_calls==4);
  setup(t); ant[1].team=t; food[10][9]=1;
  step_ant(&ant[0]); assert(ant[0].x==9 && rng_calls==4);
  // A mature colony still sends distant soldiers on the offensive.
  setup(t); ant[0].x=20; ncnt[t]=thr[t]; food[10][21]=1;
  step_ant(&ant[0]); assert(ant[0].x==19 && rng_calls==4);
  // A soldier who is carrying a berry must deliver it, not pursue.
  setup(t); ant[0].x=9; ant[0].carry=1; ant[1].x=8;
  colony_home_threats(ant,nestx,nesty,defend_i);
  step_ant(&ant[0]); assert(ant[0].x==10 && !ant[0].carry &&
      stock[t]==1 && rng_calls==4);
}
int main(void) {
  check_team(0);
  check_team(1);
  puts("GBC live soldier step: both teams, early defense, stale threats, food delivery, offensive threshold PASS");
  return 0;
}
"""
with tempfile.TemporaryDirectory(prefix="gbc-defense-") as td:
    path = Path(td) / "defense.c"
    exe = Path(td) / "defense"
    path.write_text(FIXTURE + "\nstatic void step_ant(Ant *a) {" +
                    step + "\n" + TESTS)
    cc = shlex.split(os.environ.get("HOSTCC", "cc"))
    subprocess.run(cc + ["-std=c99", "-Wall", "-Wextra", "-Werror",
                         "-pedantic", "-I" + str(ROOT / "src"),
                         str(path), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

print("GBC defense: one shared threat scan per colony cycle, no added movement RNG draws")
