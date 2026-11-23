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
