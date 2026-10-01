"""Shared helpers for the Game Boy Color asset generators."""
import numpy as np

def rgb555(r, g, b):
    return (int(r) >> 3) | ((int(g) >> 3) << 5) | ((int(b) >> 3) << 10)

def to8(c5):
    """5-bit channel -> 8-bit, the way an emulator/LCD shows it"""
    return (c5 << 3) | (c5 >> 2)

def unpack555(v):
    return to8(v & 31), to8((v >> 5) & 31), to8((v >> 10) & 31)

def enc_tile(px):
    """8x8 array of 0..3  ->  16 bytes of GB 2bpp planar tile data (bit 7 = leftmost pixel)"""
    out = bytearray()
    for y in range(8):
        lo = hi = 0
        for x in range(8):
            v = int(px[y][x])
            lo |= (v & 1) << (7 - x)
            hi |= ((v >> 1) & 1) << (7 - x)
        out += bytes([lo, hi])
    return bytes(out)

def c_bytes(name, data, per=16, decl='const unsigned char'):
    data = bytes(data)
    s = '%s %s[%d] = {\n' % (decl, name, max(1, len(data)))
    if not data:
        return s + '0\n};\n'
    for i in range(0, len(data), per):
        s += ', '.join('0x%02x' % b for b in data[i:i + per]) + ',\n'
    return s + '};\n'

def c_words(name, vals, per=8):
    vals = list(vals)
    s = 'const unsigned short %s[%d] = {\n' % (name, len(vals))
    for i in range(0, len(vals), per):
        s += ', '.join('0x%04x' % v for v in vals[i:i + per]) + ',\n'
    return s + '};\n'

BAYER4 = (np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]], np.float32) + 0.5) / 16.0

W_RGB = np.array([0.30, 0.55, 0.15], np.float32)      # perceptual-ish channel weights

def _dist(a, b):
    d = a[..., None, :] - b
    return (d * d * W_RGB).sum(-1)

def kmeans_colors(pix, k, iters=12):
    """Lloyd k-means on pixels (n,3); initialised on luminance quantiles"""
    pix = pix.astype(np.float32)
    lum = pix @ W_RGB
    order = np.argsort(lum)
    cent = pix[order[((np.arange(k) + 0.5) * len(pix) / k).astype(int)]].copy()
    for _ in range(iters):
        a = _dist(pix, cent).argmin(-1)
        for j in range(k):
            m = a == j
            if m.any():
                cent[j] = pix[m].mean(0)
    return cent

def snap5(c):
    return (np.clip(np.round(c), 0, 255).astype(np.int32) >> 3) << 3

def tile_err(tiles, pal):
    """sum over the tile of squared distance to the nearest palette colour"""
    return _dist(tiles, pal.astype(np.float32)).min(-1).sum(-1)

def fit_palettes(tiles, npal, ncol=4, iters=14):
    """tiles (N,64,3) -> (palettes (npal,ncol,3) snapped to 5 bit, assignment (N,)).
    The usual GBC recipe: cluster tiles, fit a palette to each cluster, reassign, repeat."""
    tiles = tiles.astype(np.float32)
    means = tiles.mean(1)
    grp = _dist(means, kmeans_colors(means, npal)).argmin(-1)
    pals = np.zeros((npal, ncol, 3), np.float32)
    for it in range(iters):
        for p in range(npal):
            sel = tiles[grp == p].reshape(-1, 3)
            if len(sel) == 0:                              # empty cluster: steal the worst-fit tile
                pals_i = np.array([tile_err(tiles, pals[q]) if it else np.zeros(len(tiles)) for q in range(p)])
                sel = tiles[np.random.default_rng(p + it).integers(len(tiles))].reshape(-1, 3)
            pals[p] = kmeans_colors(sel, ncol)
        pals = snap5(pals).astype(np.float32)
        errs = np.stack([tile_err(tiles, pals[p]) for p in range(npal)], 1)
        grp = errs.argmin(1)
    return pals.astype(np.int32), grp

def dither_tile(tile, pal):
    """tile (64,3) float, pal (ncol,3) -> 8x8 indices using an ordered dither between the two nearest colours"""
    pal = pal.astype(np.float32)
    out = np.zeros((8, 8), np.uint8)
    for i in range(64):
        c = tile[i]
        d = ((pal - c) ** 2 * W_RGB).sum(-1)
        o = np.argsort(d)
        a, b = o[0], o[1]
        v = pal[b] - pal[a]
        den = (v * v * W_RGB).sum()
        t = 0.0 if den == 0 else float(np.clip(((c - pal[a]) * v * W_RGB).sum() / den, 0, 1))
        y, x = divmod(i, 8)
        out[y, x] = b if t > BAYER4[y & 3, x & 3] else a
    return out
