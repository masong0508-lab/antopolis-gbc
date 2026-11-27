#!/usr/bin/env python3
"""Ensure the tested GBC worker-routing rule is wired into the live simulation.

Pure host tests are insufficient if a future refactor stops using the
helper; this source guard verifies the carrier branch, the unchanged
searcher/soldier behavior, and the same in-game pheromone history.
"""
from pathlib import Path

source = Path("src/main.c").read_text()
assert '#include "colony_route.h"' in source
begin = source.index("static void step_ant(Ant *a) {")
end = source.index("// Game logic is sliced:", begin)
logic = source[begin:end]
assert "ph[a->y][a->x] = p > 215 ? 255 : p + 40" in logic
assert "if (a->carry) sc = colony_home_score(" in logic
assert "ph[ty][tx], (uint8_t)rand()" in logic
assert "else if (soldier)" in logic
assert "sc = colony_forager_score(ph[ty][tx], food[ty][tx]," in logic
assert "(uint8_t)rand()" in logic
assert "sc = ph[ty][tx] + (food[ty][tx] ? 120 : 0)" not in logic
assert "if (found) { a->x += DX[bd]; a->y += DY[bd]; }" in logic
assert "if (a->carry && a->x == nestx[t] && a->y == nesty[t])" in logic
print("GBC routing wired into actual carrying ants; searcher and soldier AI preserved")

# Compile the live arrival condition rather than a duplicated scoring rule.
# Check both colonies, priority of food, defense and siege, and free motion.
import os
import re
import shlex
import subprocess
import tempfile

arrival = re.search(r"  if \(a->carry \?.*?goto target_reached;", logic, re.S)
assert arrival, "missing live arrival guard"
assert logic.index("ph[a->y][a->x] =") < arrival.start()
assert arrival.end() < logic.index("for (d = 0; d < 4; d++)")
assert logic.index("target_reached:") < logic.index("if (a->carry && a->x == nestx[t]")
program = r"""
#include <stdint.h>
#include <assert.h>

typedef struct { uint8_t x,y,carry; } Ant;
static uint8_t nestx[2]={2,29}, nesty[2]={3,28};
static int hold(Ant *a, uint8_t t, uint8_t soldier, uint8_t goal_x, uint8_t goal_y) {

""" + arrival.group() + r"""
  return 0;
target_reached:
  return 1;
}

int main(void) {
  for (uint8_t t=0;t<2;++t) {
    uint8_t e=t^1;
    Ant a={nestx[t],nesty[t],1};
    assert(hold(&a,t,1,12,13));
    a.x=12; a.y=13;
    assert(!hold(&a,t,1,12,13)); /* food outranks the intruder */
    a.carry=0;
    assert(hold(&a,t,1,12,13));
    assert(!hold(&a,t,0,12,13));
    a.x++;
    assert(!hold(&a,t,1,12,13));
    a.x=nestx[e]; a.y=nesty[e];
    assert(hold(&a,t,1,nestx[e],nesty[e]));
    assert(!hold(&a,t,1,12,13));
    assert(!hold(&a,t,0,nestx[e],nesty[e]));
  }
  return 0;
}

"""
with tempfile.TemporaryDirectory() as tmp:
    src = Path(tmp) / "arrival.c"
    exe = Path(tmp) / "arrival"
    src.write_text(program)
    subprocess.run(shlex.split(os.environ.get("HOSTCC", "cc")) +
                   ["-std=c99", "-Wall", "-Wextra", "-Werror", str(src), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Live colony arrival guard: workers, defense and siege passed")
