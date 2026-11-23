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
assert "p & (J_SELECT | J_UP | J_DOWN)" in pin, "Nest jump must be edge-triggered"
assert "colony_cursor_warp(jump, pl, nestx, nesty, &cx, &cy)" in pin
assert "d->su = 1;" in pin, "A warp must not become an accidental flood"
assert 'CURSOR: HOME NEST' in pin and 'CURSOR: RIVAL NEST' in pin
assert "if (d->sl && !d->su) flood();" in pin, "Keep ordinary SELECT flood"
assert 'SEL UP HOME DN RIVAL' in help_text, "Teach both new shortcuts"
print("GBC cursor shortcut wired: both nests, held-key suppression, flood safety")
