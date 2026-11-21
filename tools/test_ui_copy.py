#!/usr/bin/env python3
"""Check GBC bank-2 UI text before it is copied to the 640-byte scratch RAM."""
import ast
import re
from pathlib import Path

SOURCE = Path("src/text2.c").read_text()
VALID = set(" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/!<>")
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
assert help_text.startswith("#START HERE\n")
assert "SEL START: ANT EYE" in help_text
assert "CURSOR TILE IN HUD" in help_text

cards = strings_for("TCARD2").split("\0")
assert len(cards) == CARDS, f"Expected {CARDS} tutorial cards, got {len(cards)}"
for i, card in enumerate(cards):
    check_page(f"TCARD2[{i}]", card)
print(f"GBC UI copy PASS: {len(help_text)} help bytes, {len(cards)} cards, all <=20 columns")
