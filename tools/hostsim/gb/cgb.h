#pragma once
typedef uint16_t palette_color_t;
#define RGB(r,g,b) ((r)|((g)<<5)|((b)<<10))
void set_bkg_palette(uint8_t,uint8_t,const palette_color_t*); void set_sprite_palette(uint8_t,uint8_t,const palette_color_t*);
