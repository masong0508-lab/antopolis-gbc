// DippInn logo sequence for Game Boy Color, ~8 s.  Artwork comes from tools/make_dippinn_logo.py.
//   Phase 1 (frames 0-160):   dusk scene laid out like the reference - blue sky on the left, wavy light ridge in the
//                             middle, dark speckled hills + big moon on the right (noise tiles rotate = scrolling noise),
//                             a sun sinking behind, fast white clouds in front, slow pink clouds behind the hills.
//   Phase 2 (161-211):        grey screen + twisting scan line, cut to black
//   Phase 3 (212-478):        outlined title text fades in, rope underline grows pixel by pixel from the centre, fade out
#include <gb/gb.h>
#include <stdint.h>
#include "dippinn_logo.h"
#include "dippinn_logo_data.h"

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#ifdef HOST_TEST                                  // tools/host_test: run on a PC against a fake memory map
volatile u8 *host_addr(u16 a);
#define R8(a) (*host_addr((u16)(a)))
#else
#define R8(a) (*(volatile u8 *)(a))
#endif
#define rLCDC R8(0xFF40)
#define rSTAT R8(0xFF41)
#define rSCY  R8(0xFF42)
#define rSCX  R8(0xFF43)
#define rLY   R8(0xFF44)
#define rLYC  R8(0xFF45)
#define rVBK  R8(0xFF4F)
#define rBCPS R8(0xFF68)
#define rBCPD R8(0xFF69)
#define rOCPS R8(0xFF6A)
#define rOCPD R8(0xFF6B)
#define LCDC_ON     0x80
#define LCDC_BG8000 0x10
#define LCDC_OBJ16  0x04
#define LCDC_OBJON  0x02
#define LCDC_BGON   0x01
#define LOGO_FRAMES (359 + 120)   /* +120 frames (2 s) hold before the fade-out */

#define P2 161
#define P3 212
#define HOLD_EXTRA 120           // +2 s: logo stays fully visible this many extra frames before the fade to black

static void lcd_off(void) {
    if (rLCDC & LCDC_ON) {
        while (rLY < 144);                       // only switch the LCD off during VBlank
        rLCDC &= (u8)~LCDC_ON;
    }
}
static void vcopy(u16 dst, const u8 *src, u16 n) {      // LCD must be off (or VRAM otherwise accessible)
    while (n--) R8(dst++) = *src++;
}
static void vfill(u16 dst, u8 v, u16 n) {
    while (n--) R8(dst++) = v;
}
static u16 fade_col(u16 c, u8 lvl) {
    u8 m = (u8)(16 - lvl);
    u16 r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    r = (r * m) >> 4; g = (g * m) >> 4; b = (b * m) >> 4;
    return r | (g << 5) | (b << 10);
}
static void bpal_write(const u16 *c, u8 first, u8 n, u8 lvl) {
    rBCPS = 0x80 | (u8)(first * 2);              // bit 7: auto-increment
    while (n--) {
        u16 v = lvl ? fade_col(*c, lvl) : *c; c++;
        while (rSTAT & 2);                       // palette RAM is locked while the LCD reads OAM/VRAM
        rBCPD = (u8)v;
        while (rSTAT & 2);
        rBCPD = (u8)(v >> 8);
    }
}

static u8 last_lvl;
static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static void hide_all_sprites(void) { u8 i; for (i = 0; i < 40; i++) hide_sprite(i); }

// ---- Phase 1: scene ----
// Scene colours are sparse (palette slot + colour). The faded colours are computed while the picture is drawn
// (pbuf) and only copied to palette RAM in VBlank, at most PAL_PER_VBL colours per frame, so nothing tears.
#define PAL_PER_VBL 12
static u16 pbuf[40];
static u8 pw, plvl_hw, plvl_want;                 // colours written for the current level, level on the hardware, wanted level
static void pal_put(u8 i) {
    u8 s = dl_sc_cslot[i]; u16 v = pbuf[i];
    if (s & 0x80) { rOCPS = 0x80 | (u8)((s & 31) * 2); rOCPD = (u8)v; rOCPD = (u8)(v >> 8); }
    else          { rBCPS = 0x80 | (u8)(s * 2);        rBCPD = (u8)v; rBCPD = (u8)(v >> 8); }
}
static void pal_calc(u8 lvl) {
    u8 i;
    for (i = 0; i < dl_sc_ncol; i++) pbuf[i] = lvl ? fade_col(dl_sc_ccol[i], lvl) : dl_sc_ccol[i];
}
static void pal_flush(void) {                     // call right after wait_vbl_done()
    u8 n = 0;
    while (pw < dl_sc_ncol && n < PAL_PER_VBL) { pal_put(pw); pw++; n++; }
}
static void pal_plan(void) {                      // call during the picture: start the next level once the last one landed
    if (pw >= dl_sc_ncol && plvl_want != plvl_hw) { plvl_hw = plvl_want; pal_calc(plvl_hw); pw = 0; }
}

#define NPINK 4
#define NWHITE 3
static const u8 pk_x0[NPINK] = {81, 2, 19, 53}, pk_y0[NPINK] = {16, 40, 54, 61};
static const u8 wh_x0[NWHITE] = {155, 51, 101}, wh_y0[NWHITE] = {55, 68, 80};   // screen x at frame 24 = reference

static u8 rbuf[96];
static void rot_calc(int f) {                     // noise tiles 1..6 rotated left by (f*2/3) px: the noise scrolls
    u8 i, s = (u8)(((f * 2) / 3) & 7);
    for (i = 0; i < 96; i++) { u8 v = dl_rot_base[i]; rbuf[i] = (u8)((v << s) | (v >> (8 - s))); }
}

static void scene_init(void) {
    u8 r, c;
    lcd_off();
    vfill(0x9800, 0, 1024); rVBK = 1; vfill(0x9800, 0, 1024); rVBK = 0;
    vcopy(0x8000, dl_sc_tiles, dl_sc_tile_count * 16);
    vcopy(0x8800, dl_spr_tiles, 36 * 16);                     // sprite tiles 128..163
    for (r = 0; r < 18; r++) vcopy(0x9800 + r * 32, dl_sc_map + r * 32, 32);
    rVBK = 1;                                                 // sky + ridge: band palette per 3 rows; hills + moon: palette 7
    for (r = 0; r < 18; r++) for (c = 0; c < 32; c++) R8(0x9800 + r * 32 + c) = c < 13 ? (u8)(r / 3) : 7;
    rVBK = 0;
    hide_all_sprites();
    last_lvl = 16; plvl_want = plvl_hw = 16; pal_calc(16);
    for (pw = 0; pw < dl_sc_ncol; pw++) pal_put(pw);
    rot_calc(0);
    rSCX = rSCY = 0;
    rLCDC = LCDC_ON | LCDC_BG8000 | LCDC_OBJ16 | LCDC_OBJON | LCDC_BGON;
}
static void place(u8 nb, int x, int y, u8 tile, u8 pal) {   // one 8x16 sprite, x/y = screen position of its top-left
    if (x < -7 || x > 159 || y < -15 || y > 143) { hide_sprite(nb); return; }
    set_sprite_tile(nb, tile); set_sprite_prop(nb, pal);
    move_sprite(nb, (u8)(x + 8), (u8)(y + 16));
}
static void scene_frame(int f) {
    u8 i, k, lvl;
    int fadein = f < 15 ? 16 - f * 16 / 15 : 0;                       // 0.25 s fade in
    int dim = (12 * clampi((f - 84) * 256 / 78, 0, 256)) >> 8;        // dim to night from 1.4 s over 1.3 s
    int m = (f * 3) >> 2, x, y;
    pal_flush();                                                       // VBlank work first
    if (pw >= dl_sc_ncol) set_data((u8 *)0x8010, rbuf, 96);            // rotated noise tiles (skipped while a palette is in flight)
    lvl = (u8)(fadein > dim ? fadein : dim);
    plvl_want = lvl;
    for (i = 0; i < NWHITE; i++) {                                     // white clouds: fast, in front of everything
        x = (int)((wh_x0[i] + 380 - m) % 190) - 28; y = wh_y0[i];
        for (k = 0; k < 3; k++) place(3 * i + k, x + 8 * k, y, 140 + 6 * i + 2 * k, 1);
    }
    y = 25 + ((f - 24) * 27) / 20;                                     // sun sinks ~1.35 px per frame
    place(9, 33, y, 158, 2); place(10, 41, y, 160, 2);
    place(11, 96 - (f >> 2), 56, 162, 3);                              // small dark-blue ball
    for (i = 0; i < NPINK; i++) {                                      // pink clouds: slow, behind ridge + hills (priority bit)
        x = (int)((pk_x0[i] + 358 - (f >> 2)) % 176) - 16;
        for (k = 0; k < 2; k++) place(12 + 2 * i + k, x + 8 * k, pk_y0[i], 128 + 4 * (i % 3) + 2 * k, 0x80);
    }
    pal_plan(); rot_calc(f + 1);
}

// ---- Phase 2: grey screen + scan line ----
static void grey_init(void) {
    lcd_off();
    rSCX = rSCY = 0;
    hide_all_sprites();
    vfill(0x9800, 0, 1024); rVBK = 1; vfill(0x9800, 0, 1024); rVBK = 0;
    vcopy(0x8000, dl_gr_tiles, 4 * 16);
    last_lvl = 12; bpal_write(dl_gr_pal, 0, 4, 12);
    rLCDC = LCDC_ON | LCDC_BG8000 | LCDC_BGON;
}
static void grey_frame(int f) {
    u8 c, lvl, t;
    int lf = f - P2;
    int l = lf < 15 ? 12 - lf * 12 / 15 : 0;                           // haze clears
    if (f >= 197) l = (f - 197) * 16 / 15;                             // cut to black
    lvl = (u8)clampi(l, 0, 16);
    if (lvl != last_lvl) { last_lvl = lvl; bpal_write(dl_gr_pal, 0, 4, lvl); }
    for (c = 0; c < 20; c++) {                                         // rope-like twist on the left, flat elsewhere (row 10 = y 80)
        t = c < 9 ? ((lf & 1) ? 3 : 2) : 1;
        R8(0x9800 + 10 * 32 + c) = t;
    }
}

// ---- Phase 3: text + underline ----
// Text = tile rows 8-9, rope = tile row 10 (palette 1). The rope grows one pixel at a time: the cells between the two
// fronts use one of 4 phase tiles (period-4 pattern), the two front cells use tiles built every frame (RT_L / RT_R).
static u8 rope_tx;                                   // first rope tile id (4 phase tiles, then left / right front tile)
static u8 rope_cells[16], rope_tl[16], rope_tr[16], rope_dirty;
static u8 rope_lo, rope_hi, rope_fr;
static void rope_tile(u8 *out, u8 p0, u8 lo, u8 hi, u8 fr) {          // rope px p0..p0+7, visible lo..hi, pattern phase fr
    u8 i, k, v, l, h; int p;
    for (i = 0; i < 16; i++) out[i] = 0;
    for (k = 0; k < 4; k++) {
        l = h = 0;
        for (i = 0; i < 8; i++) {
            p = p0 + i;
            if (p < lo || p > hi) continue;
            if (p == 0 || p == 127) v = k ? 1 : 0;                      // end caps
            else v = dl_rope_pat[k * 4 + (((p + 2 + fr) & 3))];
            l |= (u8)((v & 1) << (7 - i)); h |= (u8)((v >> 1) << (7 - i));
        }
        out[(4 + k) * 2] = l; out[(4 + k) * 2 + 1] = h;
    }
}
static void rope_build(u8 lo, u8 hi, u8 fr) {                          // fills the per-frame buffers (called outside VBlank)
    u8 t, cl = lo >> 3, cr = hi >> 3;
    for (t = 0; t < 16; t++) rope_cells[t] = 0;
    if (lo > hi) { rope_dirty = 0; return; }
    for (t = cl + 1; t < cr; t++) rope_cells[t] = (u8)(rope_tx + fr);
    rope_cells[cl] = (u8)(rope_tx + 4); rope_tile(rope_tl, (u8)(cl << 3), lo, hi, fr);
    if (cr != cl) { rope_cells[cr] = (u8)(rope_tx + 5); rope_tile(rope_tr, (u8)(cr << 3), lo, hi, fr); }
    rope_dirty = (u8)(cr != cl ? 2 : 1);
}
static void text_init(void) {
    u8 r, f;
    lcd_off();
    vfill(0x9800, 0, 1024); rVBK = 1; vfill(0x9800, 0, 1024); rVBK = 0;
    vcopy(0x8000, dl_tx_tiles, dl_tx_tile_count * 16);
    rope_tx = (u8)dl_tx_tile_count;
    for (f = 0; f < 4; f++) { rope_tile(rope_tl, 8, 0, 255, f); vcopy(0x8000 + (rope_tx + f) * 16, rope_tl, 16); }   // interior phase tiles
    for (r = 0; r < 2; r++) vcopy(0x9800 + (8 + r) * 32, dl_tx_map + r * 20, 20);
    rVBK = 1; vfill(0x9800 + 10 * 32 + 2, 1, 16); rVBK = 0;            // rope row uses palette 1
    last_lvl = 16; bpal_write(dl_tx_pal, 0, 8, 16);
    rope_dirty = 0; rope_lo = 255; rope_hi = 0; rope_fr = 255;
    rLCDC = LCDC_ON | LCDC_BG8000 | LCDC_BGON;
}
static void text_frame(int f) {
    u8 i, lvl, fr, lo, hi;
    int lf = f - P3, l = 0, half, ease, L, u;
    if (rope_dirty) {                                                  // VBlank: copy last frame's buffers
        for (i = 0; i < 16; i++) set_vram_byte((u8 *)(0x9800 + 10 * 32 + 2 + i), rope_cells[i]);
        set_data((u8 *)(0x8000 + (rope_tx + 4) * 16), rope_tl, 16);
        if (rope_dirty == 2) set_data((u8 *)(0x8000 + (rope_tx + 5) * 16), rope_tr, 16);
        rope_dirty = 0;
    }
    if (lf < 42) l = 16 - lf * 16 / 42;                                // 0.7 s text fade-in
    if (lf >= 110 + HOLD_EXTRA) l = (lf - 110 - HOLD_EXTRA) * 16 / 36; // 0.6 s fade to black
    lvl = (u8)clampi(l, 0, 16);
    if (lvl != last_lvl) { last_lvl = lvl; bpal_write(dl_tx_pal, 0, 8, lvl); }
    L = clampi((lf - 18) * 256 / 54, 0, 256); u = 256 - L;             // rope grows after 0.3 s over 0.9 s, cubic ease-out
    ease = 256 - (int)((((u32)u * u >> 8) * u) >> 8);
    half = (64 * ease) >> 8;                                           // 0..64 px each side of the centre
    fr = (u8)((lf / 5) & 3);
    lo = half ? (u8)(64 - half) : 255; hi = half ? (u8)(63 + half) : 0;
    if (lf >= 0 && (lo != rope_lo || hi != rope_hi || fr != rope_fr)) {
        rope_lo = lo; rope_hi = hi; rope_fr = fr;
        rope_build(lo, hi, fr);
    }
}

static void logo_play(void) {
    int f;
    scene_init();
    for (f = 0; f < LOGO_FRAMES; f++) {
        wait_vbl_done();
        if (f > 20 && (joypad() & J_START)) break;                     // START skips the logo
        if (f == P2) grey_init();
        if (f == P3) text_init();
        if (f < P2) scene_frame(f); else if (f < P3) grey_frame(f); else text_frame(f);
    }
    wait_vbl_done();
    lcd_off();                                                         // black, everything off
    hide_all_sprites();
}

void dippinn_logo_play(void) {
    logo_play();                                                       // returns with the LCD off
    rVBK = 0; rSCX = 0; rSCY = 0;
    hide_all_sprites();
    rLCDC = 0x00;                                                      // fully off: main() does its own setup
}
