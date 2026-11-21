#!/usr/bin/env python3
"""Check GBC bank-2 UI text before it is copied to the 640-byte scratch RAM."""
import ast
import re
from pathlib import Path

SOURCE = Path("src/text2.c").read_text()
# The real 5x7 GBC font does not provide '<' or '>' glyphs.
VALID = set(" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/!")
MAX_SCRATCH = 640
CARDS = 10


def strings_for(name):
    match = re.search(r"const char " + name + r"\[\]\s*=(.*?);", SOURCE, re.S)
    assert match, f"Missing {name}"
    literals = re.findall(r'"(?:\\.|[^"\\])*"', match.group(1))
    assert literals, f"Empty {name}"
    return "".join(ast.literal_eval(s) for s in literals)


def check_page(name, page):
    assert len(page.encode("ascii")) + 1 <= MAX_SCRATCH, (
        f"{name}: text needs {len(page)+1} bytes, scratch is {MAX_SCRATCH}"
    )
    for y, row in enumerate(page.splitlines(), 1):
        heading = row.startswith("#") and name == "HELP2"
        shown = row[1:] if heading else row
        assert len(shown) <= 20, f"{name} row {y} overflows 20 tiles: {shown!r}"
        for ch in shown:
            assert ch in VALID, f"{name} row {y}: unsupported font glyph {ch!r}"


help_text = strings_for("HELP2")
check_page("HELP2", help_text)
assert help_text.startswith("#YOUR MISSION\n")
for instruction in ("SAVE YOUR QUEEN", "DEFEAT THE RED QUEEN",
                    "BUILD ROUTES TO FOOD", "SEL START ANT EYE",
                    "#ANT EYE 3D", "#HUD LABELS", "G GRASS H HILL",
                    "#ELECTIONS"):
    assert instruction in help_text, f"Missing up-to-date help instruction: {instruction}"

cards = strings_for("TCARD2").split("\0")
assert len(cards) == CARDS, f"Expected {CARDS} tutorial cards, got {len(cards)}"
for i, card in enumerate(cards):
    check_page(f"TCARD2[{i}]", card)
# The world editor's successful actions used to be silent except for a
# generic sound, leaving players unsure what the landscape just became.
# Check the *shipping C action handlers*, including each of the six
# terrain results, so these labels cannot silently regress into codes.
map_source = Path("src/main.c").read_text()
raise_handler = map_source.split("static void raise_land(void) {", 1)[1].split(
    "static void lower_land(void) {", 1
)[0]
lower_handler = map_source.split("static void lower_land(void) {", 1)[1].split(
    "// mana workflow", 1
)[0]
for heading, handler, phrases in (
    ("RAISE", raise_handler, ("WATER FILLED", "RAMP BUILT", "HILL RAISED")),
    ("LOWER", lower_handler, ("HILL CUT DOWN", "RAMP LOWERED", "MOAT DUG")),
):
    assert "draw_cell(cx, cy)" in handler and "say(" in handler, (
        f"{heading}: successful terrain edit lost feedback or tile refresh"
    )
    assert "if (!hgt[cy][cx]) food[cy][cx] = 0;" in lower_handler, (
        "Lowering a berry tile into water must remove the food"
    )
    assert "FOOD WASHED AWAY" in lower_handler, (
        "Food-loss warning must remain visible in the GBC HUD"
    )
    for phrase in phrases:
        assert f'"{phrase}"' in handler, f"{heading}: missing feedback {phrase}"
        assert len(phrase) <= 20 and all(ch in VALID for ch in phrase), (
            f"{heading}: unsupported 5x7 status row {phrase}"
        )
print(f"GBC UI copy PASS: {len(help_text)} help bytes, {len(cards)} cards, all <=20 columns")
