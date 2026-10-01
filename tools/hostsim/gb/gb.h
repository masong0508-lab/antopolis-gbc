#include <stdint.h>
#include <stdlib.h>
#define J_START 1
#define J_SELECT 2
#define J_A 4
#define J_B 8
#define J_LEFT 16
#define J_RIGHT 32
#define J_UP 64
#define J_DOWN 128
extern uint8_t DIVV; extern uint8_t SCXV,SCYV,NRV,LCDV,VBKV;
#define DIV_REG DIVV
#define SCX_REG SCXV
#define SCY_REG SCYV
#define LCDC_REG LCDV
#define VBK_REG VBKV
#define VBK_TILES 0
#define VBK_ATTRIBUTES 1
#define LCDCF_WIN9C00 0x40
#define LCDCF_BG9C00 0x08
#define NR10_REG NRV
#define NR11_REG NRV
#define NR12_REG NRV
#define NR13_REG NRV
#define NR14_REG NRV
#define NR21_REG NRV
#define NR22_REG NRV
#define NR23_REG NRV
#define NR24_REG NRV
#define NR41_REG NRV
#define NR42_REG NRV
#define NR43_REG NRV
#define NR44_REG NRV
#define NR50_REG NRV
#define NR51_REG NRV
#define NR52_REG NRV
void vsync(void); uint8_t joypad(void); void waitpad(uint8_t); void waitpadup(void);
void set_bkg_tile_xy(uint8_t,uint8_t,uint8_t); void set_win_tile_xy(uint8_t,uint8_t,uint8_t);
void set_bkg_data(uint8_t,uint8_t,const uint8_t*);
void set_sprite_data(uint8_t,uint8_t,const uint8_t*); void set_sprite_tile(uint8_t,uint8_t);
void set_sprite_prop(uint8_t,uint8_t); void move_sprite(uint8_t,uint8_t,uint8_t); void move_win(uint8_t,uint8_t);
#define HIDE_SPRITES
#define SHOW_SPRITES
#define SHOW_BKG
#define SHOW_WIN
#define DISPLAY_ON
#define DISPLAY_OFF
#define SPRITES_8x8
