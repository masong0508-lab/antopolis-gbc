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
assert "rx = eye_ray_increment(angle, c);" in ray
assert "ry = eye_ray_increment((uint8_t)(angle + 12u), c);" in ray
assert "eye_mul_small(" not in ray
assert "if (vcol >= VX && dirty) { vcol = 0; dirty = 0; }" in sweep_source
print("ANT EYE ray camera latched across progressive sweep")

# Verify the exact production compass index expression against the real
# eight-way movement vectors at every one of the sixteen camera headings.
import re
nav_source = Path("src/eye_navigation.h").read_text()
def vector(name):
    return [int(n) for n in re.search(name + r"\[16\]\s*=\s*\{([^}]+)\}", nav_source).group(1).split(",")]
dx, dy = vector("EYE_DX"), vector("EYE_DY")
ui_source = Path("src/eye_ui.c").read_text()
assert "eye_ui_ahead((char *)tbuf + 480, d, vang);" in eye
assert "put_str(18, 0, (const char *)tbuf + 501);" in eye
assert "COMPASS[" not in eye
labels = re.findall(r'"([A-Z ]{2})"', re.search(r"COMPASS\[8\]\s*=\s*\{([^}]+)\}", ui_source).group(1))
index_expr = re.search(r"const char \*heading = COMPASS\[(.*?)\];", ui_source).group(1)
index_expr = re.sub(r"(?<=\d)u\b", "", index_expr).replace("angle","vang")
for vang in range(16):
    expected = ("N" if dy[vang]<0 else "S" if dy[vang]>0 else "") + ("W" if dx[vang]<0 else "E" if dx[vang]>0 else "")
    assert labels[eval(index_expr, {"__builtins__": {}}, {"vang": vang})].strip() == expected, (vang, expected)
print("All sixteen ANT EYE HUD bearings match actual walking directions")

# The 175-byte perspective table stays in the same ROM bank as the
# ray marcher. Verify every entry against the original signed formula.
perspective = re.search(r"static const int8_t EYE_OFFSETS\[7\]\[25\]\s*=\s*\{(.*?)\n\};", render, re.S)
assert perspective
rows = [[int(n) for n in re.findall(r"-?\d+", row)]
        for row in re.findall(r"\{([^{}]+)\}", perspective.group(1))]
assert len(rows)==7 and all(len(row)==25 and row[0]==0 for row in rows)
for height_index in range(7):
    for depth in range(1,25):
        offset=int(((height_index-3)*4-3)*16/depth)
        assert rows[height_index][depth]==max(-64,min(64,offset))
assert "yt = 48 - EYE_OFFSETS[h + 3 - eh][n];" in ray
assert "eye_init_offsets" not in game+render
assert not re.search(r"int8_t\s+OFF\s*\[",game+render)
print("All 175 perspective entries verified; no duplicate WRAM table or boot copy")

# Compare all 640 direction components with the original signed shifts.
ray_table = re.search(r"static const int16_t EYE_RAY_X\[320\]\s*=\s*\{(.*?)\n\};", render, re.S)
assert ray_table
ray_values = [int(n) for n in re.findall(r"-?\d+", ray_table.group(1))]
assert len(ray_values) == 320
sine16 = [0,24,45,59,64,59,45,24,0,-24,-45,-59,-64,-59,-45,-24]
for angle in range(16):
    for col in range(20):
        t = col*2-19
        expected_x = (sine16[(angle+4)&15]*32 - sine16[angle]*t) >> 4
        expected_y = (sine16[angle]*32 + sine16[(angle+4)&15]*t) >> 4
        assert ray_values[angle*20+col] == expected_x
        assert ray_values[((angle+12)&15)*20+col] == expected_y
assert "EYE_RAY_X[(row << 4) + (row << 2) + column]" in render
print("All 640 original signed ray direction components match ROM lookup")
