#!/usr/bin/env python3
"""Guard the *shipping GBC input loop*, not merely the pure cursor helper."""
from pathlib import Path

source = Path("src/main.c").read_text()
help_text = Path("src/text2.c").read_text()
assert '#include "colony_cursor.h"' in source
pin = source.split("static void pin(uint8_t k, Pad *d) {", 1)[1].split(
    "static void pass_card(void)", 1
)[0]
assert "nest_chord" in pin
assert "if (!nest_chord && dirs)" in pin, "Held chord must not move cursor"
assert "if (nest_chord)" in pin, "Held chord must keep cursor fixed"
assert "colony_cursor_target(pl, (uint8_t)!(k & J_UP))" in pin
assert "cx = nestx[team]; cy = nesty[team];" in pin
assert "d->su = 1;" in pin, "A warp must not become an accidental flood"
assert "uint8_t nest_chord" in pin
assert "if (d->sl && !d->su) flood();" in pin, "Keep ordinary SELECT flood"
assert 'SEL UP HOME DN RIVAL' in help_text, "Teach both new shortcuts"
print("GBC compact cursor shortcut wired: both nests, held-key suppression, flood safety")
