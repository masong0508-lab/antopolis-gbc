#!/usr/bin/env python3
"""Keep sprite-type refresh immediate without making animation costly."""
from pathlib import Path

main = Path("src/main.c").read_text()
render = Path("src/eye_render.c").read_text()
header = Path("src/sprite_logic.h").read_text()
makefile = Path("Makefile").read_text()

body = main.split("static void draw_sprites(void) {", 1)[1].split(
    "// ---------- screens ----------", 1
)[0]
assert "colony_walker_needs_refresh(" in body
assert "shadow_OAM[ANTSPR + s].tile =" in body
assert "colony_walker_frame(s, (uint8_t)(walker_redraw_clock >> 2), tbuf)" in body
assert body.index("tbuf[s] = ant[i].carry;") < body.index(
    "colony_walker_needs_refresh("
) < body.index("set_sprite_prop(ANTSPR + s,")
assert "if (!(++walker_redraw_clock & 3u))" in body, (
    "Retain the original quarter-rate bank switch for walking animation"
)
assert body.count("eye_animate_walkers(") == 1
assert body.count("disable_interrupts();") == 1
assert "colony_walker_frame(" in render
assert "colony_walker_needs_refresh" in header
assert "tools/test_sprite_logic.c" in makefile
assert "tools/test_sprite_wiring.py" in makefile
print("GBC sprite wiring: immediate carrier correction, same animation cadence and OAM slots")
