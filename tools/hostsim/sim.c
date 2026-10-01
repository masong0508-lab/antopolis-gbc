#ifndef LIMIT
#define LIMIT 40000
#endif
// Host-side simulator: stubs the GBDK API so the game logic can be run, fuzzed and balance-tested
// on a PC. Not a Game Boy emulator: it checks logic, bounds and balance, not graphics/timing/sound.
// usage: ./sim MODE DIV SEED   MODE 0 idle, 1 wander+spam buttons, 2 sweep whole map
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <gb/gb.h>
#include <gb/cgb.h>
#define rand gb_rand
uint32_t g_extra; uint8_t DIVV,SCXV,SCYV,NRV,LCDV,VBKV;
static unsigned long frame; static int stuck_chk;
void vsync(void); uint8_t joypad(void);
void waitpad(uint8_t a){(void)a;} void waitpadup(void){}
void set_bkg_tile_xy(uint8_t x,uint8_t y,uint8_t t){ if(x>=32||y>=32) {printf("BKG OOB\n");exit(1);} (void)t;}
void set_win_tile_xy(uint8_t x,uint8_t y,uint8_t t){ if(x>=32||y>=32||(VBKV==0&&(t<136||t>=176))) {printf("WIN BAD %d %d %d\n",x,y,t);exit(1);} }
void set_bkg_data(uint8_t a,uint8_t b,const uint8_t*c){(void)a;(void)b;(void)c;}
void set_sprite_data(uint8_t a,uint8_t b,const uint8_t*c){(void)a;(void)b;(void)c;}
void set_sprite_tile(uint8_t a,uint8_t b){(void)a;(void)b;}
void set_sprite_prop(uint8_t a,uint8_t b){(void)a;(void)b;}
void move_win(uint8_t a,uint8_t b){(void)a;(void)b;}
void set_bkg_palette(uint8_t a,uint8_t b,const palette_color_t*c){(void)a;(void)b;(void)c;}
void set_sprite_palette(uint8_t a,uint8_t b,const palette_color_t*c){(void)a;(void)b;(void)c;}
void move_sprite(uint8_t s,uint8_t x,uint8_t y){ if(s>=37){printf("SPR OOB\n");exit(1);} (void)x;(void)y;}
#define main game_main
#include "../../src/main.c"
#undef main
static int mode;
uint8_t joypad(void){
  if(frame<3) return J_START;
  uint8_t k=0; unsigned f=frame;
  if(mode==1){ // wander cursor everywhere, spam A/B/SELECT
    k=(f/40%4==0)?J_RIGHT:(f/40%4==1)?J_DOWN:(f/40%4==2)?J_LEFT:J_UP;
    if(f%7==0)k|=J_A; if(f%11==0)k|=J_B; if(f%97==0)k|=J_SELECT; if(f%53==0)k|=J_START;
  }
  if(mode==2){k=(f/300%4==0)?J_RIGHT:(f/300%4==1)?J_DOWN:(f/300%4==2)?J_LEFT:J_UP;}
  if(over) k|=J_START;
  return k;
}
static unsigned mind[2]={99,99}; static unsigned firstend=0; static int endwho=0; static uint8_t minq0=9,minq1=9,maxcx,maxcy,mincx=99,mincy=99; static unsigned wins,losses;
void vsync(void){
  frame++;
  if(qhp[0]&&qhp[0]<minq0)minq0=qhp[0]; if(qhp[1]&&qhp[1]<minq1)minq1=qhp[1];
  if(cx>maxcx)maxcx=cx; if(cy>maxcy)maxcy=cy; if(cx<mincx)mincx=cx; if(cy<mincy)mincy=cy;
  { unsigned i; for(i=0;i<MAXA;i++) if(ant[i].alive){ unsigned e=ant[i].team^1; unsigned d=dist(ant[i].x,nestx[e])+dist(ant[i].y,nesty[e]); if(d<mind[ant[i].team])mind[ant[i].team]=d; } }
  if(over&&!firstend){firstend=frame;endwho=over;}
  if(over==1)wins++; if(over==2)losses++;
  if(frame==LIMIT){ printf("FIRSTEND frame %u (%.0f s) result %d\n",firstend,firstend/60.0,endwho); printf("closest approach to enemy nest: black %u red %u\n",mind[0],mind[1]); printf("min queen hp seen: me %u foe %u | cursor range x %u-%u y %u-%u | win/lose frames %u/%u\n",minq0,minq1,mincx,maxcx,mincy,maxcy,wins,losses); }
  if(frame%1000000==0){
    printf("f=%lu tk=%u ants %u/%u stock %u/%u queen %u/%u mana %u cursor %u,%u cam %u,%u scroll %d,%d over %u\n",frame,tk,ncnt[0],ncnt[1],stock[0],stock[1],qhp[0],qhp[1],mana,cx,cy,camx,camy,scx,scy,over);
  }
  if(scx<0||scy<0||scx>96||scy>128){printf("SCROLL OOB\n");exit(1);}
  if(cx>=W||cy>=H){printf("CURSOR OOB\n");exit(1);}
  if(frame>LIMIT) exit(0);
}
int main(int argc,char**argv){ DIVV=argc>2?atoi(argv[2]):0; if(argc>3) g_extra=atoi(argv[3]); mode=argc>1?atoi(argv[1]):0; game_main(); return 0; }
