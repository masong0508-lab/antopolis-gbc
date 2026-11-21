#!/usr/bin/env python3
"""Validate the real GBC pause/help text before it reaches tbuf[640].

The GBDK text2() copier is deliberately minimal and has no runtime bound
check because ROM bank 0 is almost full. A wider or longer string would
corrupt rendering/world scratch RAM. Catch that in the host build instead.
"""
import ast
import re
from pathlib import Path

source = (Path(__file__).resolve().parents[1] / "src" / "text2.c").read_text()
start = source.index("const char HELP2[] =")
end = source.index("const char TCARD2[] =", start)
section = source[start:end]
literals = re.findall(r'^\\s*("(?:\\\\.|[^"\\\\])*")', section, re.MULTILINE)
assert literals, "No C string literals in HELP2"
help_text = "".join(ast.literal_eval(literal) for literal in literals)
assert help_text.endswith("\\n"), "Help rows must be newline terminated"
assert "\\0" not in help_text, "HELP2 must have one continuous page"
size = len(help_text.encode("ascii")) + 1
assert size <= 640, f"HELP2 uses {size} bytes, exceeds tbuf[640]"
rows = help_text.splitlines()
assert len(rows) <= 99, "Page counter only has two digits"
for number, line in enumerate(rows, 1):
    visible = line[1:] if line.startswith("#") else line
    assert len(visible) <= 20, f"Help row {number} exceeds 20 tiles: {line!r}"
    assert all(c == " " or c in "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:!/" for c in visible), (
        f"Help row {number} uses unsupported raised-font glyphs: {line!r}"
    )
for required in ("#YOUR MISSION", "#MAP CONTROLS", "#ANT EYE 3D",
                 "#HUD LABELS", "#ELECTIONS", "SAVE YOUR QUEEN",
                 "DEFEAT THE RED QUEEN", "A RAISE LAND 1 MANA"):
    assert required in rows, f"Missing playable instruction: {required}"
print(f"GBC help text passed: {size}/640 bytes, {len(rows)} readable rows")
