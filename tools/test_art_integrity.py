#!/usr/bin/env python3
"""Verify source ant sprite sketches match committed 2bpp Game Boy tiles."""
import ast
import re
from pathlib import Path

source = ast.parse(Path("tools/make_art.py").read_text())
sprite_source = next(
    item.value for item in source.body
    if isinstance(item, ast.Assign)
    and any(isinstance(x, ast.Name) and x.id == "SP" for x in item.targets)
)
rows = ast.literal_eval(sprite_source)
assert len(rows) == 56, "Seven 8x8 sprite tiles: worker/cursor/queen, walking and carrying frames"
assert all(len(s) == 8 and set(s) <= set("0123") for s in rows)

encoded = []
for row in rows:
    lo = hi = 0
    for ch in row:
        value = int(ch)
        lo = (lo << 1) | (value & 1)
        hi = (hi << 1) | ((value >> 1) & 1)
    encoded.extend((lo, hi))

content = Path("src/art.h").read_text()
m = re.search(r"static const uint8_t SPT\[112\]\s*=\s*\{(.*?)\};", content, re.S)
assert m, "GBC generated sprite tile table missing"
actual = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", m.group(1))]
assert len(actual) == 112, f"SPT contains {len(actual)}, expected 112 bytes"
assert actual == encoded, "Hand-drawn source art and GBC tile bytes are out of sync"
assert rows[0:8] != rows[16:24], "Worker and queen must look distinct"
assert rows[24:32] != rows[32:40], "Walking animation frames must differ"
assert rows[40:48] != rows[48:56], "Carrying animation frames must differ"
assert rows[40:48] != rows[24:32], "Carrying worker must look unlike empty worker"
assert all("3" in row for row in rows[40:43]), "Carried food must be high contrast"
# The walking tiles were previously only used on the title screen:
# verify the actual gameplay sprite pass consumes both distinct frames.
gameplay = Path("src/main.c").read_text()
sprite_pass = gameplay[gameplay.index("static void draw_sprites(void) {"):]
sprite_pass = sprite_pass[:sprite_pass.index("// ---------- screens ----------")]
assert "eye_animate_walkers(s," in sprite_pass, (
    "Overhead colony ants no longer animate their walking silhouettes"
)
assert "sys_time & 3u" in sprite_pass, (
    "Sprite tile writes lost their low-overhead four-frame cadence"
)
banked = Path("src/eye_render.c").read_text()
assert "void eye_animate_walkers(uint8_t visible, uint8_t phase," in banked
assert "const uint8_t *carrying) BANKED" in banked
assert "carrying[i >> 3]" in banked
assert "5u + frame : 3u + frame" in banked
assert "set_sprite_data(0, 7, SPT)" in gameplay
assert "carrying[s >> 3]" in sprite_pass
assert "shadow_OAM[4u + i].tile =" in banked, (
    "Walker update must use only OAM 4..39, reserving queen/player slots"
)
assert "#define QSPR 0" in gameplay and "#define ANTSPR 4" in gameplay, (
    "Queen/cursor priorities must beat crowded worker scanlines"
)
assert "move_sprite(ANTSPR + s," in sprite_pass, (
    "Overhead workers must not overwrite reserved queen OAM slots"
)
assert "disable_interrupts();" in sprite_pass and "enable_interrupts();" in sprite_pass, (
    "Switchable-ROM walker animation must guard the bank-1 audio ISR"
)
for i in (0, 2, 3, 4, 5, 6):
    opaque = sum(ch != "0" for row in rows[i*8:(i+1)*8] for ch in row)
    assert 18 <= opaque <= 52, f"Sprite {i} is empty or a solid square"
print("GBC ant sprite source, queen silhouette, animation and tile bytes PASS")
