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

# The shipping ray sweep must latch a single camera pose on column zero.
# Gameplay continues using live controls, and a dirty sweep schedules a refresh.
sweep_source = Path("src/main.c").read_text()
assert "if (!vcol) { render_x = vx; render_y = vy; render_angle = vang; }" in sweep_source
assert "view_cols(render_x, render_y, vcol, (uint8_t)(vcol + 2), render_angle);" in sweep_source
assert "view_col(c, vx, vy, eh, angle);" in sweep_source
ray = render.split("void view_col(",1)[1].split("void eye_animate_walkers(",1)[0]
assert "SIN16[angle]" in ray and "SIN16[vang]" not in ray
assert "if (vcol >= VX && dirty) { vcol = 0; dirty = 0; }" in sweep_source
print("ANT EYE ray camera latched across progressive sweep")
