"""DippInn logo artwork -> Game Boy Color tiles / maps / palettes (src/dippinn_logo_data.c)
Run: python3 tools/make_dippinn_logo.py
Three phases, each with its own tile set (loaded into VRAM bank 0 while the LCD is off):
  scene : sky (flat tile rows, 6 blues), wavy light ridge, speckled dark hills, sprite clouds + moon
  grey  : flat grey screen + scan line (3 tiles)
  text  : "DippInn Productions" (5x9 font, outline) + 4-frame twisted rope underline"""
import sys, math
sys.path.insert(0, 'tools')
import numpy as np
from gbc_common import *

def hashn(x, y):
    n = (x * 374761393 + y * 668265263) & 0xFFFFFFFF
    n = ((n ^ (n >> 13)) * 1274126177) & 0xFFFFFFFF
    return (n ^ (n >> 16)) & 255

def flat(i): return enc_tile(np.full((8, 8), i, np.uint8))
tile_sets = {}

# ------------------------------------------------------------------ scene
sky = [rgb555(20 + b * 9, 50 + b * 14, 190 + b * 8) for b in range(6)]
HL, FILL = rgb555(0x8f, 0xc0, 0xf5), rgb555(0x4f, 0x92, 0xe8)
D1, D2, D3 = rgb555(2, 4, 28), rgb555(8, 16, 60), rgb555(20, 34, 100)
sc_pal = sky[0:4] + [sky[4], sky[5], HL, FILL] + [sky[5], D1, D2, D3]     # pal0 flat sky, pal1 sky/ridge, pal2 hills

def ridge_y(x):
    return max(73, 82 + int(round(6 * math.sin(2 * math.pi * x / 128) + 3 * math.sin(2 * math.pi * x / 64 + 1.0))))
def hill_top(x):
    return 116 + int(round(11 * math.sin(2 * math.pi * x / 128 + 0.6) + 5 * math.sin(2 * math.pi * x / 64 + 2.0)))

sc_tiles = []; sc_lookup = {}
def sc_add(data):
    if data not in sc_lookup: sc_lookup[data] = len(sc_tiles); sc_tiles.append(data)
    return sc_lookup[data]
MAP_W, MAP_H = 32, 18
sc_map = bytearray(); sc_attr = bytearray()
for r in range(MAP_H):
    for c in range(MAP_W):
        if r < 8:                                            # flat sky, two tile rows per band (16 px)
            sc_map.append(sc_add(flat(r // 2 % 4))); sc_attr.append(0)
        elif r == 8:
            sc_map.append(sc_add(flat(0))); sc_attr.append(1)
        elif r < 12:                                         # ridge
            px = np.zeros((8, 8), np.uint8)
            for yy in range(8):
                for xx in range(8):
                    x, y = c * 8 + xx, r * 8 + yy
                    y0 = ridge_y(x)
                    px[yy, xx] = 0 if y < y0 else 2 if y < y0 + 2 else 3
            sc_map.append(sc_add(enc_tile(px))); sc_attr.append(1)
        else:                                                # hills (BG priority bit 7: colours 1-3 hide the clouds)
            px = np.zeros((8, 8), np.uint8)
            for yy in range(8):
                for xx in range(8):
                    x, y = c * 8 + xx, r * 8 + yy
                    if y >= hill_top(x):
                        n = hashn(x % 128, y)
                        px[yy, xx] = 1 if n < 46 else 3 if n > 183 else 2
            sc_map.append(sc_add(enc_tile(px))); sc_attr.append(2 | 0x80)
assert len(sc_tiles) <= 128, len(sc_tiles)                   # sprite tiles live at 128+

# sprites: 8x16 mode. cloud shape i = tiles 128+4i.. (left top,left bottom,right top,right bottom); moon = 140..143
CL = [[(2, 2, 10, 4), (0, 4, 14, 4), (4, 0, 6, 3)], [(0, 2, 8, 3), (3, 0, 7, 3), (6, 3, 9, 3)], [(1, 1, 12, 3), (0, 3, 16, 4), (5, 0, 6, 2)]]
spr = []
for i in range(3):
    bm = np.zeros((16, 16), np.uint8)
    for (x0, y0, w, h) in CL[i]: bm[y0:y0 + h, x0:x0 + w] = 1
    for half in range(2):                                    # left / right sprite
        for t in range(2):                                   # top / bottom tile
            spr.append(enc_tile(bm[t * 8:t * 8 + 8, half * 8:half * 8 + 8]))
bm = np.zeros((16, 16), np.uint8)
for y in range(16):
    for x in range(16):
        if (2 * x - 15) ** 2 + (2 * y - 15) ** 2 <= 108: bm[y, x] = 1
for half in range(2):
    for t in range(2):
        spr.append(enc_tile(bm[t * 8:t * 8 + 8, half * 8:half * 8 + 8]))
spr_pal = [0, rgb555(0xc7, 0x7f, 0xb8), 0, 0, 0, rgb555(0x2a, 0x3f, 0x9a), 0, 0]

# ------------------------------------------------------------------ grey phase
gr_pal = [rgb555(0x2B, 0x29, 0x33), rgb555(0xbd, 0xb8, 0xcc), rgb555(0x6b, 0x67, 0x84), 0]
gr = [flat(0)]
for k in range(3):                                           # 0 flat, 1 twist0, 2 twist1  (tile 1..3)
    px = np.zeros((8, 8), np.uint8)
    for x in range(8):
        l = [0, (x >> 1) & 1, ((x >> 1) + 1) & 1][k]
        px[l, x] = 1; px[l + 1, x] = 2
    gr.append(enc_tile(px))

# ------------------------------------------------------------------ text phase
FONT = {
 'D': ["####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####.", ".....", "....."],
 'P': ["####.", "#...#", "#...#", "####.", "#....", "#....", "#....", ".....", "....."],
 'I': [".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###.", ".....", "....."],
 'i': ["..#..", ".....", ".##..", "..#..", "..#..", "..#..", ".###.", ".....", "....."],
 'p': [".....", ".....", "####.", "#...#", "#...#", "#...#", "####.", "#....", "#...."],
 'n': [".....", ".....", "####.", "#...#", "#...#", "#...#", "#...#", ".....", "....."],
 'r': [".....", ".....", "#.##.", "##..#", "#....", "#....", "#....", ".....", "....."],
 'o': [".....", ".....", ".###.", "#...#", "#...#", "#...#", ".###.", ".....", "....."],
 'd': ["....#", "....#", ".####", "#...#", "#...#", "#...#", ".####", ".....", "....."],
 'u': [".....", ".....", "#...#", "#...#", "#...#", "#..##", ".##.#", ".....", "....."],
 'c': [".....", ".....", ".###.", "#...#", "#....", "#...#", ".###.", ".....", "....."],
 't': [".....", ".#...", "####.", ".#...", ".#...", ".#..#", "..##.", ".....", "....."],
 's': [".....", ".....", ".####", "#....", ".###.", "....#", "####.", ".....", "....."],
}
TX_ROW0 = 8                                                  # text canvas = tile rows 8..10 (y 64..87)
ROPE_ROW = 11
canvas = np.zeros((24, 160), np.uint8)
mask = np.zeros((24, 160), np.uint8)
s = 'DippInn Productions'; adv = sum(4 if ch == ' ' else 6 for ch in s)
cx = (160 - adv) // 2
for ch in s:
    if ch == ' ': cx += 4; continue
    for y in range(9):
        for x in range(5):
            if FONT[ch][y][x] == '#': mask[7 + y, cx + x] = 1     # glyph rows y=71..79 (text centred on y=76)
    cx += 6
for y in range(24):
    for x in range(160):
        v = 1 if mask[y, x] else 0
        if not v:
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    yy, xx = y + dy, x + dx
                    if 0 <= yy < 24 and 0 <= xx < 160 and mask[yy, xx]: v = 2
        canvas[y, x] = v
tx_pal = [0, rgb555(128, 110, 215), rgb555(14, 9, 30), 0,
          0, rgb555(0x8a, 0x7a, 0xe0), rgb555(0x4a, 0x3f, 0x8f), rgb555(0xd9, 0xd2, 0xff)]
tx = [flat(0)]; tx_lookup = {tx[0]: 0}
def tx_add(data):
    if data not in tx_lookup: tx_lookup[data] = len(tx); tx.append(data)
    return tx_lookup[data]
tx_map = bytearray()
for r in range(3):
    for c in range(20):
        tx_map.append(tx_add(enc_tile(canvas[r * 8:r * 8 + 8, c * 8:c * 8 + 8])))
rope_map = bytearray()
for f in range(4):
    strip = np.zeros((8, 128), np.uint8)
    for x in range(126):
        w = ((x >> 1) + f) & 1
        strip[2 + w, x + 1] = 1; strip[4 - w, x + 1] = 2
    for y in range(2, 5): strip[y, 0] = 3; strip[y, 127] = 3
    for c in range(16):
        rope_map.append(tx_add(enc_tile(strip[:, c * 8:c * 8 + 8])))

# ------------------------------------------------------------------ sine table (Q7), used for cloud bobbing and the title drift
sin_q7 = bytes([(int(round(127 * math.sin(2 * math.pi * i / 256)))) & 255 for i in range(256)])

with open('src/dippinn_logo_data.c', 'w') as o:
    o.write('// GENERATED by tools/make_dippinn_logo.py - do not edit\n#include "dippinn_logo_data.h"\n')
    o.write('const unsigned int dl_sc_tile_count = %d;\n' % len(sc_tiles))
    o.write(c_bytes('dl_sc_tiles', b''.join(sc_tiles)))
    o.write(c_bytes('dl_sc_map', sc_map, 32)); o.write(c_bytes('dl_sc_attr', sc_attr, 32))
    o.write(c_words('dl_sc_pal', sc_pal))
    o.write(c_bytes('dl_spr_tiles', b''.join(spr)))
    o.write(c_words('dl_spr_pal', spr_pal))
    o.write(c_bytes('dl_gr_tiles', b''.join(gr))); o.write(c_words('dl_gr_pal', gr_pal))
    o.write('const unsigned int dl_tx_tile_count = %d;\n' % len(tx))
    o.write(c_bytes('dl_tx_tiles', b''.join(tx))); o.write(c_bytes('dl_tx_map', tx_map, 20)); o.write(c_bytes('dl_rope_map', rope_map, 16))
    o.write(c_words('dl_tx_pal', tx_pal))
    o.write('const signed char dl_sin_q7[256] = {\n')
    sv = [b - 256 if b > 127 else b for b in sin_q7]
    for i in range(0, 256, 16): o.write(', '.join('%d' % v for v in sv[i:i + 16]) + ',\n')
    o.write('};\n')
print('scene tiles %d (max 128), sprite tiles %d, text-phase tiles %d, bytes ~%d' %
      (len(sc_tiles), len(spr), len(tx), 16 * (len(sc_tiles) + len(spr) + len(gr) + len(tx)) + 2 * 576 + 256))
