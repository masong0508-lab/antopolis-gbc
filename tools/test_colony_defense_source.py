#!/usr/bin/env python3
"""Protect GBC defense integration and prevent costly per-ant world scans."""
from pathlib import Path

source = Path("src/main.c").read_text()
makefile = Path("Makefile").read_text()
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
assert "goal_x = enemy->x; goal_y = enemy->y;" in step
assert "de = dist(tx, goal_x) + dist(ty, goal_y);" in step
assert "sc = -14 * (int16_t)de + (rand() & 31);" in step, "Keep soldier RNG and attack rate"
assert "colony_home_threat(" not in step, "No full ant scan for each soldier"
assert tick.count("colony_home_threat(") == 2
assert tick.index("if (++slice < 8) return;") < tick.index("colony_home_threat(")
assert "defend_i[0] = defend_i[1] = MAXA;" in newgame
assert "tools/test_colony_defense.c" in makefile
print("GBC defense wired once per colony cycle; regular rendering and RNG preserved")
