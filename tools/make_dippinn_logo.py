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

# ------------------------------------------------------------------ scene (vertical layout like the reference)
# cols 0-6 sky (6 banded palettes), cols 7-12 wavy ridge, cols 13-19 dark noisy hills + big moon bottom-right.
# BG tile 0 = flat sky, tiles 1-4 = rotating hill noise, tiles 5-6 = rotating moon noise (the C code rotates them
# a pixel per frame = scrolling noise), the rest are static (ridge, moon rim).
SKY = [(14,48,187),(24,63,197),(31,70,204),(40,87,213),(56,103,219),(69,116,232)]
MID, LIGHT, N0, BEIGE, WHITE = (74,145,235),(138,192,245),(3,8,42),(189,167,170),(228,216,246)
bg_cols = []                                               # (slot, rgb555): slot = pal*4 + colour
for p in range(6):
    for ci, col in enumerate([SKY[p], MID, LIGHT, N0]): bg_cols.append((p * 4 + ci, rgb555(*col)))
bg_cols += [(29, rgb555(*N0)), (30, rgb555(*BEIGE)), (31, rgb555(*WHITE))]   # pal 7: hills + moon
obj_cols = [(1, rgb555(199,127,184)), (2, rgb555(150,95,170)),               # pal 0 pink clouds
            (5, rgb555(236,230,250)), (6, rgb555(186,196,240)),              # pal 1 white clouds
            (9, rgb555(253,241,171)), (10, rgb555(253,169,56)), (11, rgb555(246,120,70)),   # pal 2 sun
            (13, rgb555(41,56,156))]                                          # pal 3 ball
sc_cols = [(s, c) for s, c in bg_cols] + [(0x80 | s, c) for s, c in obj_cols]

def ridge_x(y):
    return 80 + int(round(7 * math.sin(2 * math.pi * y / 64) + 3 * math.sin(2 * math.pi * y / 32 + 1.0)))
MCX, MCY, MR = 134, 124, 23
def moon_in(x, y): return (x - MCX) ** 2 + (y - MCY) ** 2 <= MR * MR

sc_tiles = []; sc_lookup = {}
def sc_add(data):
    if data not in sc_lookup: sc_lookup[data] = len(sc_tiles); sc_tiles.append(data)
    return sc_lookup[data]
def noise_tile(seed, kind):                                 # kind 0 hills (idx 1, specks idx 3), 1 moon (idx 2, specks idx 1)
    px = np.zeros((8, 8), np.uint8)
    for y in range(8):
        for x in range(8):
            n = hashn(seed * 13 + x, seed * 7 + y)
            px[y, x] = (3 if n < 11 else 1) if kind == 0 else (1 if n < 70 else 2)
    return px
rot_px = [noise_tile(i, 0) for i in range(4)] + [noise_tile(10 + i, 1) for i in range(2)]
sc_add(flat(0))
for p in rot_px: sc_add(enc_tile(p))                        # tiles 1..6 (the C code rotates these)
rot_base = b''.join(enc_tile(p) for p in rot_px)
MAP_W, MAP_H = 32, 18
sc_map = bytearray(32 * 18)
for r in range(MAP_H):
    for c in range(MAP_W):
        if c >= 20: t = 0
        elif c < 7: t = 0
        elif c < 13:                                         # ridge
            px = np.zeros((8, 8), np.uint8)
            for yy in range(8):
                for xx in range(8):
                    x, y = c * 8 + xx, r * 8 + yy
                    d = x - ridge_x(y)
                    px[yy, xx] = 0 if d < 0 else 1 if d < 5 else 2 if d < 7 else 1 if d < 10 else 3
            t = sc_add(enc_tile(px))
        else:                                                # hills / moon
            x0, y0 = c * 8, r * 8
            cnt = sum(moon_in(x0 + xx, y0 + yy) for yy in range(8) for xx in range(8))
            if cnt == 0: t = 1 + hashn(c, r) % 4
            elif cnt == 64: t = 5 + hashn(c, r) % 2
            else:
                px = np.zeros((8, 8), np.uint8)
                for yy in range(8):
                    for xx in range(8):
                        n = hashn(x0 + xx, y0 + yy)
                        px[yy, xx] = (1 if n < 70 else 2) if moon_in(x0 + xx, y0 + yy) else (3 if n < 11 else 1)
                t = sc_add(enc_tile(px))
        sc_map[r * 32 + c] = t
assert len(sc_tiles) <= 128, len(sc_tiles)

# sprites (8x16 mode). A shape of W columns = W sprites, each (top tile, bottom tile).
def rects(w, rs, shade_last=False):
    bm = np.zeros((16, w * 8), np.uint8)
    for (x0, y0, ww, hh) in rs: bm[y0:y0 + hh, x0:x0 + ww] = 1
    if shade_last:
        for x in range(w * 8):
            col = np.where(bm[:, x])[0]
            if len(col): bm[col.max(), x] = 2
    return bm
shapes = [
 rects(2, [(3,1,7,2),(0,3,11,3)], True), rects(2, [(1,0,6,2),(0,2,12,3)], True), rects(2, [(2,1,9,2),(0,3,14,3)], True),
 rects(3, [(5,1,9,3),(1,4,14,2),(0,5,20,3),(9,8,11,1)], True), rects(3, [(7,0,8,3),(2,3,16,2),(0,5,21,5)], True), rects(3, [(6,0,8,3),(4,3,14,3),(1,6,18,5)], True)]
sun = np.zeros((16, 16), np.uint8)
for y in range(16):
    for x in range(16):
        rr = math.hypot(x - 7.5, y - 7.5)
        sun[y, x] = 1 if rr <= 5 else 2 if rr <= 6.6 else (3 if (x + y) & 1 else 0) if rr <= 8 else 0
ball = np.zeros((16, 8), np.uint8)
for y in range(7):
    for x in range(7):
        if (2 * x - 6) ** 2 + (2 * y - 6) ** 2 <= 40: ball[y, x] = 1
spr = []
for bm in shapes + [sun, ball]:
    for col in range(bm.shape[1] // 8):
        for half in range(2): spr.append(enc_tile(bm[half * 8:half * 8 + 8, col * 8:col * 8 + 8]))
assert len(spr) == 36, len(spr)

# ------------------------------------------------------------------ grey phase
gr_pal = [rgb555(0x2B, 0x29, 0x33), rgb555(0xbd, 0xb8, 0xcc), rgb555(0x6b, 0x67, 0x84), 0]
gr = [flat(0)]
for k in range(3):                                           # 0 flat, 1 twist0, 2 twist1  (tile 1..3)
    px = np.zeros((8, 8), np.uint8)
    for x in range(8):
        l = [0, (x >> 1) & 1, ((x >> 1) + 1) & 1][k]
        px[l, x] = 1; px[l + 1, x] = 2
    gr.append(enc_tile(px))

# ------------------------------------------------------------------ text phase (pixels copied from the reference video)
import json
ref = json.load(open('tools/ref_text.json'))
TX_ROW0 = 8                                                  # text canvas = tile rows 8,9 (y 64..79); rope row = tile row 10 (rows 4-7)
canvas = np.zeros((16, 160), np.uint8)
tmap = {'.': 0, 'P': 1, 'N': 2, 'M': 3, 'd': 3, 'C': 3}
for y, row in enumerate(ref['T']):
    for x, ch in enumerate(row): canvas[y, 14 + x] = tmap[ch]
tx_pal = [0, rgb555(131,103,213), rgb555(3,0,40), rgb555(128,113,173),
          0, rgb555(128,114,173), rgb555(3,0,40), rgb555(72,63,105)]
tx = [flat(0)]; tx_lookup = {tx[0]: 0}
def tx_add(data):
    if data not in tx_lookup: tx_lookup[data] = len(tx); tx.append(data)
    return tx_lookup[data]
tx_map = bytearray()
for r in range(2):
    for c in range(20):
        tx_map.append(tx_add(enc_tile(canvas[r * 8:r * 8 + 8, c * 8:c * 8 + 8])))
# rope pattern (period 4 px, 4 rows), idx per x%4 -> [x%4==0..3]; pal1: 1 = light, 2 = navy, 3 = dark
ROPE = [[2, 0, 0, 2], [1, 2, 2, 1], [2, 3, 3, 2], [3, 0, 0, 3]]      # rows 87..90 of the reference; x%4 = 3,0 / 1,2 pairs

# ------------------------------------------------------------------ sine table (Q7), used for cloud bobbing and the title drift
sin_q7 = bytes([(int(round(127 * math.sin(2 * math.pi * i / 256)))) & 255 for i in range(256)])

with open('src/dippinn_logo_data.c', 'w') as o:
    o.write('// GENERATED by tools/make_dippinn_logo.py - do not edit\n#include "dippinn_logo_data.h"\n')
    o.write('const unsigned int dl_sc_tile_count = %d;\n' % len(sc_tiles))
    o.write(c_bytes('dl_sc_tiles', b''.join(sc_tiles)))
    o.write(c_bytes('dl_sc_map', sc_map, 32))
    o.write(c_bytes('dl_sc_cslot', bytes(s for s, c in sc_cols))); o.write(c_words('dl_sc_ccol', [c for s, c in sc_cols]))
    o.write('const unsigned char dl_sc_ncol = %d;\n' % len(sc_cols)); o.write(c_bytes('dl_rot_base', rot_base))
    o.write(c_bytes('dl_spr_tiles', b''.join(spr)))
    o.write(c_bytes('dl_gr_tiles', b''.join(gr))); o.write(c_words('dl_gr_pal', gr_pal))
    o.write('const unsigned int dl_tx_tile_count = %d;\n' % len(tx))
    o.write(c_bytes('dl_tx_tiles', b''.join(tx))); o.write(c_bytes('dl_tx_map', tx_map, 20)); o.write(c_bytes('dl_rope_pat', bytes(v for row in ROPE for v in row)))
    o.write(c_words('dl_tx_pal', tx_pal))
    o.write('const signed char dl_sin_q7[256] = {\n')
    sv = [b - 256 if b > 127 else b for b in sin_q7]
    for i in range(0, 256, 16): o.write(', '.join('%d' % v for v in sv[i:i + 16]) + ',\n')
    o.write('};\n')
print('scene tiles %d (max 128), sprite tiles %d, text-phase tiles %d, bytes ~%d' %
      (len(sc_tiles), len(spr), len(tx), 16 * (len(sc_tiles) + len(spr) + len(gr) + len(tx)) + 2 * 576 + 256))
