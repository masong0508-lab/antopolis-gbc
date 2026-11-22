#!/usr/bin/env python3
"""Check the game actually uses the banked incremental occupancy code.

Native differential unit coverage alone would not catch accidentally
retaining the old 1,024-cell work inside the real input handlers.
"""
from pathlib import Path
game = Path("src/main.c").read_text()
module = Path("src/eye_occupancy.c").read_text()
render = Path("src/eye_render.c").read_text()
pilot = game.split("static uint8_t eye_pilot(",1)[1].split("static uint8_t eye_bite(",1)[0]
bite = game.split("static uint8_t eye_bite(",1)[1].split("static void eye(void)",1)[0]
eye = game.split("static void eye(void)",1)[1].split("// ---------- TUTORIAL",1)[0]
assert "eye_occ_pair(oldx, oldy, vx, vy)" in pilot, "Movement must refresh only old/new tiles"
assert "eye_occ_pair(tx, ty, tx, ty)" in bite, "Bite must refresh only actual target"
assert "memset(occ" not in pilot + bite, "No full occupancy atlas reset per input"
assert "occ_set()" not in pilot + bite, "No 1,024-cell food scan per input"
assert "eye_occ_init();" in eye, "Build atlas once on entry via ROM3"
assert "memset(occ, 0, sizeof occ); occ_set();" not in eye
assert "memset(occ, 0, sizeof occ);" in eye, "Clean occupancy before leaving"
assert "eye_occ_init" in module and "eye_occ_pair" in module
assert "#pragma bank 3" in module
assert "occ[y][x] = food[y][x] ? 5u : 0u;" in module, (
    "Full initialization must clear empty cells as well as restore food")
assert "occ[" in render, "The native first-person view must use this exact atlas"
print("PASS: GBC ROM3 initializes occupancy once, then refreshes only touched tiles")
