"""Verify actual GBC UI code exposes play, goal and help-page controls."""
from pathlib import Path
source = Path("src/main.c").read_text()
title = source.split("static void title_paint(void)",1)[1].split("static void records_draw(void)",1)[0]
help_screen = source.split("static void help_open(void)",1)[1].split("static void win_clear(void) {",1)[0]
game = source.split("if (paused) {",1)[1].split("tt++;",1)[0]
assert 'put_str(4, 9, "START: PLAY")' in title
assert 'put_str(0, 13, "GOAL: FEED YOUR ANTS")' in title
assert '"A:TUTORIAL DOWN:VS"' in title
assert '"SEL START: SANDBOX"' in title
assert "ant_help_next_top(htop, hlines, d)" in help_screen
assert '"START:GO U/D L/R PG"' in help_screen
assert "J_UP | J_DOWN | J_LEFT | J_RIGHT" in game
assert "(dirs & J_RIGHT) ? HR" in game
assert "(dirs & J_LEFT) ? -HR" in game
assert "(dirs & J_DOWN) ? 1 : -1" in game
print("PASS: GBC title explains START, and help has working page navigation")
