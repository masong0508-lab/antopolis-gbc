/* Low-cost GBC HUD formatting in switchable ROM3.
   Caller uses ANT EYE scratch bytes 480..500, outside antenna masks. */
#pragma bank 3
#include <gb/gb.h>
#include <stdint.h>
static void blank(char *b){uint8_t i;for(i=0;i<20;i++)b[i]=' ';b[20]=0;}
static void field(char *b,uint8_t x,const char *s){while(*s&&x<20)b[x++]=*s++;}
static void digits(char *b,uint8_t x,uint8_t v){
  uint8_t n=0;if(v>99)v=99;while(v>=10){v-=10;n++;}
  b[x]='0'+n;b[x+1]='0'+v;
}
// Feedback codes are shared with the fixed-bank input loop:
 // 1 blocked, 2 picked up, 3 delivered, 4 missed, 5 enemy, 6 queen.
void eye_ui_notice(char *b,uint8_t result) BANKED {
  static const char *const messages[7]={
    "", "PATH BLOCKED", "FOOD PICKED UP", "FOOD DELIVERED",
    "BITE MISSED", "ENEMY DEFEATED", "QUEEN HIT!"
  };
  blank(b);
  if(result<7) field(b,0,messages[result]);
}
void eye_ui_ahead(char *b,uint8_t target) BANKED {
  static const char *const names[9]={
    "EDGE","WATER","CLIFF","OPEN","FOOD","ALLY","ENEMY","OUR QUEEN","RED QUEEN"
  };
  blank(b);field(b,0,"AHEAD: ");field(b,7,names[target<9?target:0]);
}
void eye_ui_guide(char *b,uint8_t nav,uint8_t carry,
                  uint8_t x,uint8_t y,uint8_t hx,uint8_t hy) BANKED {
  uint8_t north,south,east,west;
  blank(b);field(b,0,carry?"HOME:":"FOOD:");
  if(carry){north=y>hy;south=y<hy;east=x<hx;west=x>hx;}
  else {north=(nav&8u)!=0;south=(nav&4u)!=0;
        east=(nav&1u)!=0;west=(nav&2u)!=0;}
  if(!carry&&!(nav&0x80u))field(b,6,"NONE");
  else if(!north&&!south&&!east&&!west)field(b,6,"HERE");
  else {b[6]=north?'N':south?'S':' ';b[7]=east?'E':west?'W':' ';}
  b[11]='X';digits(b,12,x);b[15]='Y';digits(b,16,y);
}
