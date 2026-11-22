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
# Verify every terrain/biome source pixel against what is actually
# shipped in the .gbc, not merely the seven character tiles.
def art_array(name):
    node = next(
        item.value for item in source.body
        if isinstance(item, ast.Assign)
        and any(isinstance(x, ast.Name) and x.id == name for x in item.targets)
    )
    return ast.literal_eval(node)

def packed_bytes(pixel_rows):
    data = []
    for row in pixel_rows:
        assert len(row) == 8 and set(row) <= set("0123")
        lo = hi = 0
        for digit in row:
            value = int(digit)
            lo = (lo << 1) | (value & 1)
            hi = (hi << 1) | ((value >> 1) & 1)
        data.extend((lo, hi))
    return data

def assert_art_table(name, pixels):
    expected = packed_bytes(pixels)
    match = re.search(
        rf"static const uint8_t {name}\[{len(expected)}\]\s*=\s*\{{(.*?)\}};",
        content, re.S
    )
    assert match, f"{name} missing or incorrect tile count"
    current = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", match.group(1))]
    assert current == expected, f"{name}: generated GBC tile data is stale"

biomes = art_array("BG")
bar = art_array("BAR")
details = art_array("ALT")
assert len(biomes) == 48 and len(bar) == 16
assert len(details) == 8 and all(len(tile) == 8 for tile in details)
assert_art_table("BGT", biomes + bar)
assert_art_table("BGT_ALT", [line for tile in details for line in tile])
title = art_array("TART")
assert len(title) == 96, "Twelve title tiles must retain the original VRAM footprint"
assert_art_table("TAT", title)
# Composite a/b/c/d lunar quadrants; previously tile b was blank, so the
# GBC title displayed a missing 8x8 corner rather than a full moon.
assert all("3" in "".join(title[i*8:(i+1)*8]) for i in range(3, 7)), (
    "Title moon lost a bright quadrant"
)
assert all(any(ch == "0" for row in title[i*8:(i+1)*8] for ch in row)
           for i in range(3, 7)), "Moon lost its transparent curved silhouette"

# The stylized nests have a genuine dark tunnel entrance, not a blob
# of the same color as sand; grass berries use contrasting crop clusters.
assert "0" in "".join(biomes[4*8:5*8])
assert "1" in "".join(biomes[5*8:6*8])
assert len(set(tuple(biomes[i*8:(i+1)*8]) for i in range(6))) == 6
assert all(tuple(details[i*2]) != tuple(details[i*2+1]) for i in range(4))
# The eight replacement tiles must face the correct real-world neighbor:
# coast N/S/W/E, then cliff N/S/W/E. Transpose/mirror exactly.
def transposed(tile):
    return ["".join(tile[y][x] for y in range(8)) for x in range(8)]
for offset in (0, 4):
    north, south, west, east = details[offset:offset+4]
    assert south == list(reversed(north)), "North/south edge tiles disagree"
    assert west == transposed(north), "West edge faces the wrong side"
    assert east == transposed(south), "East edge faces the wrong side"
assert "1" in "".join(details[0]) and "0" in "".join(details[0])
assert "1" in "".join(details[4]) and "3" in "".join(details[4])

assert rows[0:8] != rows[16:24], "Worker and queen must look distinct"
assert rows[24:32] != rows[32:40], "Walking animation frames must differ"
assert rows[40:48] != rows[48:56], "Carrying animation frames must differ"
assert rows[40:48] != rows[24:32], "Carrying worker must look unlike empty worker"
assert all("3" in row for row in rows[40:42] + rows[48:50]), "Both food-carrying poses need a gold packet"
# The walking tiles were previously only used on the title screen:
# verify the actual gameplay sprite pass consumes both distinct frames.
gameplay = Path("src/main.c").read_text()
sprite_pass = gameplay[gameplay.index("static void draw_sprites(void) {"):]
sprite_pass = sprite_pass[:sprite_pass.index("// ---------- screens ----------")]
assert "eye_animate_walkers(s," in sprite_pass, (
    "Overhead colony ants no longer animate their walking silhouettes"
)
assert "++walker_redraw_clock & 3u" in sprite_pass and (
    "walker_redraw_clock >> 2" in sprite_pass
), (
    "Walking poses must advance once every four completed game redraws, "
    "not on hardware-clock edges that slow frames can skip"
)
assert "sys_time & 3u" not in sprite_pass, (
    "Do not reintroduce phase-locked hardware-clock animation freezes"
)
banked = Path("src/eye_render.c").read_text()
assert "void eye_animate_walkers(uint8_t visible, uint8_t phase," in banked
assert "const uint8_t *carrying) BANKED" in banked
frames = Path("src/sprite_logic.h").read_text()
assert "carrying[index]" in frames
assert "5u + frame : 3u + frame" in frames
assert "colony_walker_frame(i, phase, carrying)" in banked
assert "set_sprite_data(0, 7, SPT)" in gameplay
assert "terrain_edge_pick(hgt, x, y)" in gameplay
assert "refresh_terrain_neighbors(cx, cy)" in gameplay
assert "for (y = (int8_t)py - 2;" in gameplay
assert "draw_cell((uint8_t)x, (uint8_t)y)" in gameplay
assert "refresh_terrain_neighbors(cx, cy)" in gameplay
# Flood and earthquake code must redraw edges once after all edits,
# not 45 repetitive writes (nine changed cells times four neighbors).
hit = gameplay.split("static void hit(uint8_t px, uint8_t py, uint8_t q) {", 1)[1]
hit = hit.split("static void disaster(void)", 1)[0]
assert hit.count("draw_cell(") == 1 and "refresh_terrain_neighbors(" not in hit
assert "if (is_nest(x, y)) tile = BT + 4" in gameplay
assert "else if (food[y][x] && t) tile = BT + 5" in gameplay
assert "fade_level(8, 0)" in gameplay.split("void main(void)", 1)[1]
assert "set_sprite_palette(0, 7, fs)" in gameplay
assert "colony_worker_palette(ant[i].team, ant[i].carry, ant[i].sol)" in sprite_pass
assert "tbuf[s] = ant[i].carry" in sprite_pass
assert "eye_animate_walkers(s, (uint8_t)(walker_redraw_clock >> 2), tbuf)" in sprite_pass, (
    "Overhead ant animation must take its phase from completed redraws"
)
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
