// DippInn logo sequence for Game Boy Color, ~8 s.  Artwork comes from tools/make_dippinn_logo.py.
//   Phase 1 (frames 0-160):   dusk scene - flat sky bands, wavy ridge, speckled hills, sprite clouds + moon.
//                             Two scroll speeds: the LYC interrupt changes SCX at line 96 (ridge above, hills below).
//   Phase 2 (161-211):        grey screen + twisting scan line, cut to black
//   Phase 3 (212-478):        text fades in, rope underline grows from the centre (in 8 px steps), fade to black
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
#define NCLOUD 9
#define OBJ_CLOUD 128            // first sprite tile (cloud shape s = 128 + 4 s, moon = 140)
#define OBJ_MOON 140


#ifdef HOST_TEST
void lcd_off(void); void vcopy(u16 dst, const u8 *src, u16 n); void vfill(u16 dst, u8 v, u16 n);
void bpal_write(const u16 *c, u8 first, u8 n, u8 lvl); void opal_write(const u16 *c, u8 first, u8 n, u8 lvl);
#else
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
static void opal_write(const u16 *c, u8 first, u8 n, u8 lvl) {
    rOCPS = 0x80 | (u8)(first * 2);
    while (n--) {
        u16 v = lvl ? fade_col(*c, lvl) : *c; c++;
        while (rSTAT & 2);
        rOCPD = (u8)v;
        while (rSTAT & 2);
        rOCPD = (u8)(v >> 8);
    }
}
#endif

// scroll values applied by the VBlank / LYC interrupts: top zone from line 0, lower zone after line 95
#ifdef HOST_TEST
extern volatile u8 g_top_scx, g_top_scy, g_low_scx, g_low_scy;
#else
static volatile u8 g_top_scx, g_top_scy, g_low_scx, g_low_scy;
#endif
static void vbl_isr(void) { rSCX = g_top_scx; rSCY = g_top_scy; }
static void lcd_isr(void) {
    while (rSTAT & 3);                             // wait for HBlank so the change lands between two lines
    rSCX = g_low_scx; rSCY = g_low_scy;
}

static u8 last_lvl;

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static void hide_all_sprites(void) { u8 i; for (i = 0; i < 40; i++) hide_sprite(i); }

// ---- Phase 1: scene ----
static const u8 cl_x0[NCLOUD] = {2, 24, 47, 66, 90, 111, 134, 156, 178};
static const u8 cl_y0[NCLOUD] = {14, 62, 30, 94, 8, 76, 48, 24, 84};

static void scene_pal(u8 lvl) {
    bpal_write(dl_sc_pal, 0, 12, lvl);
    opal_write(dl_spr_pal, 0, 8, lvl);
}
static void scene_init(void) {
    u8 r;
    lcd_off();
    vfill(0x9800, 0, 1024); rVBK = 1; vfill(0x9800, 0, 1024); rVBK = 0;
    vcopy(0x8000, dl_sc_tiles, dl_sc_tile_count * 16);
    vcopy(0x8800, dl_spr_tiles, 16 * 16);                     // sprite tiles 128..143
    for (r = 0; r < 18; r++) vcopy(0x9800 + r * 32, dl_sc_map + r * 32, 32);
    rVBK = 1;
    for (r = 0; r < 18; r++) vcopy(0x9800 + r * 32, dl_sc_attr + r * 32, 32);
    rVBK = 0;
    hide_all_sprites();
    last_lvl = 16; scene_pal(16);
    g_top_scx = g_top_scy = g_low_scx = g_low_scy = 0;
    rSCX = rSCY = 0;
    rSTAT |= 0x40; rLYC = 95;                               // LYC interrupt after line 95: hills scroll separately
    rLCDC = LCDC_ON | LCDC_BG8000 | LCDC_OBJ16 | LCDC_OBJON | LCDC_BGON;
}
static void place(u8 nb, int x, int y, u8 tile, u8 pal) {   // one 8x16 sprite, x/y = screen position of its top-left
    if (x < -7 || x > 159 || y < -15 || y > 143) { hide_sprite(nb); return; }
    set_sprite_tile(nb, tile); set_sprite_prop(nb, pal);
    move_sprite(nb, (u8)(x + 8), (u8)(y + 16));
}
static void scene_frame(int f) {
    u8 i, lvl;
    int fadein = f < 15 ? 16 - f * 16 / 15 : 0;                       // 0.25 s fade in
    int dim = (12 * clampi((f - 84) * 256 / 78, 0, 256)) >> 8;        // dim to night from 1.4 s over 1.3 s
    lvl = (u8)(fadein > dim ? fadein : dim);
    if (lvl != last_lvl) { last_lvl = lvl; scene_pal(lvl); }
    g_top_scx = (u8)((f * 2) / 5);                                     // ridge, slow
    g_low_scx = (u8)((f * 3) >> 2);                                    // hills, faster
    for (i = 0; i < NCLOUD; i++) {                                     // pink clouds drift left, bobbing
        u16 xx = (u16)((cl_x0[i] + 400 - ((f * 3) >> 4)) % 200);
        int x = (int)xx - 30;
        int y = cl_y0[i] + ((dl_sin_q7[(u8)(f * 2 + cl_x0[i])] * 3) >> 7);
        u8 tile = OBJ_CLOUD + 4 * (i % 3);
        place(2 * i, x, y, tile, 0);
        place(2 * i + 1, x + 8, y, tile + 2, 0);
    }
    place(2 * NCLOUD, 148 - (f >> 2), 44, OBJ_MOON, 1);                // moon
    place(2 * NCLOUD + 1, 156 - (f >> 2), 44, OBJ_MOON + 2, 1);
}

// ---- Phase 2: grey screen + scan line ----
static void grey_init(void) {
    lcd_off();
    rSTAT &= (u8)~0x40;                                                // no more LYC interrupt
    g_top_scx = g_top_scy = g_low_scx = g_low_scy = 0; rSCX = rSCY = 0;
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
static u8 rope_shown, rope_frame;
static void text_init(void) {
    u8 r;
    lcd_off();
    vfill(0x9800, 0, 1024); rVBK = 1; vfill(0x9800, 0, 1024); rVBK = 0;
    vcopy(0x8000, dl_tx_tiles, dl_tx_tile_count * 16);
    for (r = 0; r < 3; r++) vcopy(0x9800 + (8 + r) * 32, dl_tx_map + r * 20, 20);
    rVBK = 1; vfill(0x9800 + 11 * 32 + 2, 1, 16); rVBK = 0;            // rope row uses palette 1
    last_lvl = 16; bpal_write(dl_tx_pal, 0, 8, 16);
    rope_shown = 0; rope_frame = 255;
    rLCDC = LCDC_ON | LCDC_BG8000 | LCDC_BGON;
}
static void text_frame(int f) {
    u8 i, lvl, vis;
    int lf = f - P3, l = 0, half, ease, L, u;
    if (lf < 42) l = 16 - lf * 16 / 42;                                // 0.7 s text fade-in
    if (lf >= 110 + HOLD_EXTRA) l = (lf - 110 - HOLD_EXTRA) * 16 / 36; // 0.6 s fade to black
    lvl = (u8)clampi(l, 0, 16);
    if (lvl != last_lvl) { last_lvl = lvl; bpal_write(dl_tx_pal, 0, 8, lvl); }
    L = clampi((lf - 18) * 256 / 54, 0, 256); u = 256 - L;             // rope grows after 0.3 s over 0.9 s, cubic ease-out
    ease = 256 - (int)((((u32)u * u >> 8) * u) >> 8);
    half = (64 * ease) >> 8;                                           // 0..64 px each side of the centre
    vis = 0;                                                           // tile i of the rope is revealed once the front reaches it
    for (i = 0; i < 16; i++) { int d = i < 8 ? 56 - 8 * i : 8 * i - 64; if (half > d && half > 0) vis++; }
    {
        u8 fr = (u8)((lf / 5) & 3);
        if (lf >= 0 && (vis != rope_shown || fr != rope_frame)) {
            rope_shown = vis; rope_frame = fr;
            for (i = 0; i < 16; i++) {
                int d = i < 8 ? 56 - 8 * i : 8 * i - 64;
                R8(0x9800 + 11 * 32 + 2 + i) = (half > d && half > 0) ? dl_rope_map[fr * 16 + i] : 0;
            }
        }
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
    disable_interrupts();
    add_VBL(vbl_isr); add_LCD(lcd_isr);
    set_interrupts(VBL_IFLAG | LCD_IFLAG);
    enable_interrupts();
    logo_play();
    disable_interrupts();                                              // hand the machine back
    remove_VBL(vbl_isr); remove_LCD(lcd_isr);
    set_interrupts(VBL_IFLAG);
    enable_interrupts();
    rSTAT &= (u8)~0x40;
    rLCDC = LCDC_BG8000 | LCDC_BGON;                                   // LCD stays off: caller does its own setup
}
