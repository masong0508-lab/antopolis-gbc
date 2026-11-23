#!/usr/bin/env python3
"""Protect live GBC nest shortcut controls and original input behaviour."""
from pathlib import Path

source = Path("src/main.c").read_text()
help_text = Path("src/text2.c").read_text()
assert '#include "colony_cursor.h"' in source
pin = source.split("static void pin(uint8_t k, Pad *d) {", 1)[1].split(
    "static void pass_card(void)", 1
)[0]
assert "(k & J_SELECT) && (dirs & (J_UP | J_DOWN))" in pin
assert "if (dirs & J_DOWN) team = colony_cursor_target(team, 1u);" in pin
assert "cx = nestx[team]; cy = nesty[team];" in pin
assert "dirs = 0;" in pin, "Held jump must suppress ordinary movement"
assert "d->su = 1;" in pin, "Releasing SELECT must not flood"
assert pin.index("if (p & J_SELECT) d->su = 0;") < pin.index(
    "if ((k & J_SELECT)") < pin.index("d->su = 1;"), (
    "SELECT reset must happen before jump consumes SELECT on the same frame"
)
assert "(d->rp % 3) == 1" in pin, "Keep the original repeat cadence"
assert "d->rp++;" in pin and "d->rp < 250" not in pin, (
    "Very long holds must not accelerate from a saturated repeat counter"
)
assert "if (d->sl && !d->su) flood();" in pin, "Keep ordinary SELECT flood"
assert "if (p & J_A) { offering();" in pin, "Keep SELECT+A trade"
assert 'SEL UP HOME DN RIVAL' in help_text
print("GBC nest warp wired with no drift/flood; original controls preserved")
