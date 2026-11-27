/* Low-cost GBC HUD formatting in switchable ROM3.
   Caller uses ANT EYE scratch bytes 480..500, outside antenna masks. */
#ifndef EYE_UI_HOST_TEST
#pragma bank 3
#include <gb/gb.h>
#else
#define BANKED
#endif
#include <stdint.h>
static void blank(char *b){uint8_t i;for(i=0;i<20;i++)b[i]=' ';b[20]=0;}
static void field(char *b,uint8_t x,const char *s){while(*s&&x<20)b[x++]=*s++;}
// Feedback codes are shared with the fixed-bank input loop:
 // 1 blocked, 2 picked up, 3 delivered, 4 missed, 5 enemy,
 // 6 queen damaged, 7 protected tutorial/sandbox queen.
void eye_ui_notice(char *b,uint8_t result) BANKED {
  static const char *const messages[8]={
    "", "BLOCKED: TURN FIRST", "FOOD PICKED UP", "FOOD DELIVERED",
    "NO ENEMY IN REACH", "ENEMY ANT DEFEATED", "RED QUEEN DAMAGED",
    "PRACTICE QUEEN SAFE"
  };
  blank(b);
  if(result<8) field(b,0,messages[result]);
}
// The existing controls row acknowledges stationary scent immediately.
// The caller updates it only when SELECT changes, using existing scratch.
void eye_ui_scent(char *b,uint8_t active) BANKED {
  blank(b); field(b,0,active?"SCENT ON   START MAP":"SEL SCENT  START MAP");
}
// Every first-person target identifies the gameplay implication, not a
// technical collision code. All messages fit the GBC's 20-tile status row.
// Writes a 20-character target row plus NUL, then a two-character bearing
// plus NUL. Caller reserves scratch bytes 480..503 (640 bytes available).
void eye_ui_ahead(char *b,uint8_t target,uint8_t angle) BANKED {
  static const char *const COMPASS[8]={"E ","SE","S ","SW","W ","NW","N ","NE"};
  const char *heading = COMPASS[((angle + ((angle & 4u) != 0)) & 15u) >> 1];
  static const char *const names[9]={
    "AHEAD: MAP EDGE", "AHEAD: WATER BLOCKS", "AHEAD: STEEP CLIFF",
    "AHEAD: CLEAR PATH", "AHEAD: FOOD WALK", "AHEAD: FRIENDLY ANT",
    "AHEAD: ENEMY BITE", "AHEAD: HOME NEST", "AHEAD: RED QUEEN"
  };
  blank(b); field(b,0,names[target<9?target:0]);
  b[21]=heading[0]; b[22]=heading[1]; b[23]=0;
}
// Check the real colony heights before announcing a homeward step.
// This is done only when the ant moves or the selected worker changes.
// No navigation map, extra ray cast, or bank-0 helper is needed.
extern uint8_t hgt[32][32];
static uint8_t ui_legal_home_leg(uint8_t x,uint8_t y,int8_t dx,int8_t dy) {
  int8_t nx=(int8_t)x+dx,ny=(int8_t)y+dy,delta;
  if(nx<0||ny<0||nx>=32||ny>=32) return 0;
  if(!hgt[y][x]||!hgt[(uint8_t)ny][(uint8_t)nx]) return 0;
  delta=(int8_t)hgt[(uint8_t)ny][(uint8_t)nx]-(int8_t)hgt[y][x];
  return (uint8_t)(delta>=-1&&delta<=1);
}
// After a proposed step, require a SECOND legal progress leg that is
// not simply the reverse. Without this check the next HUD update could
// send an ant straight back across the same blocked approach.
static uint8_t ui_home_followup(uint8_t x,uint8_t y,uint8_t hx,uint8_t hy,
                                int8_t sx,int8_t sy){
  uint8_t tx=(uint8_t)((int8_t)x+sx),ty=(uint8_t)((int8_t)y+sy);
  int8_t dx=hx>tx?1:hx<tx?-1:0;
  int8_t dy=hy>ty?1:hy<ty?-1:0;
  if(tx==hx&&ty==hy) return 1;
  return (uint8_t)((dx&&dx!=-sx&&ui_legal_home_leg(tx,ty,dx,0)) ||
                   (dy&&dy!=-sy&&ui_legal_home_leg(tx,ty,0,dy)));
}
// A nearby sidestep can get around a one-cell moat or cliff without
// claiming a full-world route. Two legal cardinal legs, no persistent
// state, no BFS queue, extra WRAM, or ray redraw. Return 1 north,
// 2 east, 3 south, 4 west, or 0 for no safe local detour.
static uint8_t ui_home_detour(uint8_t x,uint8_t y,uint8_t hx,uint8_t hy) {
  static const int8_t sx[4]={0,1,0,-1};
  static const int8_t sy[4]={-1,0,1,0};
  uint8_t i;
  for(i=0;i<4;i++)
    if(ui_legal_home_leg(x,y,sx[i],sy[i]) &&
       ui_home_followup(x,y,hx,hy,sx[i],sy[i]))
      return (uint8_t)(i+1);
  return 0;
}
// Prefer plain compass words to the old cryptic two-letter codes and X/Y
// numbers. The goal occupies a single existing row: no sprites, VRAM tiles,
// new buffers, or additional per-frame work. Only refresh on movement.
void eye_ui_guide(char *b,uint8_t nav,uint8_t carry,
                  uint8_t x,uint8_t y,uint8_t hx,uint8_t hy) BANKED {
  uint8_t north,south,east,west,pos=6;
  blank(b);
  if(carry){
    int8_t dx=hx>x?1:hx<x?-1:0;
    int8_t dy=hy>y?1:hy<y?-1:0;
    north=dy<0;south=dy>0;east=dx>0;west=dx<0;
    // Take a clear diagonal only if the destination AND both side cells
    // are legal. Otherwise prefer a reachable cardinal step over a
    // compass arrow that tells the player to walk straight into water.
    if(dx||dy) {
      if(dx&&dy&&ui_legal_home_leg(x,y,dx,dy)&&
         ui_legal_home_leg(x,y,dx,0)&&ui_legal_home_leg(x,y,0,dy)&&
         ui_home_followup(x,y,hx,hy,dx,dy)) {
        // Landing and at least one onward progress leg are safe.
      } else if(dx&&ui_legal_home_leg(x,y,dx,0) &&
                 ui_home_followup(x,y,hx,hy,dx,0)) {
        north=south=0;
      } else if(dy&&ui_legal_home_leg(x,y,0,dy) &&
                 ui_home_followup(x,y,hx,hy,0,dy)) {
        east=west=0;
      } else {
        uint8_t detour=ui_home_detour(x,y,hx,hy);
        // The word DETOUR distinguishes a verified local two-step bypass
        // from a guaranteed route to the nest (which it is not).
        if(detour==1) field(b,0,"DETOUR: NORTH");
        else if(detour==2) field(b,0,"DETOUR: EAST");
        else if(detour==3) field(b,0,"DETOUR: SOUTH");
        else if(detour==4) field(b,0,"DETOUR: WEST");
        else field(b,0,"HOME: CHECK MAP");
        return;
      }
    }
  } else {
    north=(nav&8u)!=0;south=(nav&4u)!=0;
    east=(nav&1u)!=0;west=(nav&2u)!=0;
  }
  if(!carry&&!(nav&0x80u)){field(b,0,(nav&0x40u)?"FOOD: CHECK MAP":"NO FOOD NEARBY");return;}
  field(b,0,carry?"HOME: ":"FOOD: ");
  if(!north&&!south&&!east&&!west){field(b,pos,"HERE");return;}
  if(north){field(b,pos,"NORTH");pos+=5;}
  else if(south){field(b,pos,"SOUTH");pos+=5;}
  if(east){field(b,pos,"EAST");pos+=4;}
  else if(west){field(b,pos,"WEST");pos+=4;}
  /* Show coarse map distance for both food and home. The existing
     20-cell row fits even HOME: NORTHWEST NEAR exactly. Detour and
     check-map messages above remain unchanged; these are not travel times. */
  if(carry) {
    uint8_t distance=(uint8_t)((x>hx?x-hx:hx-x)+(y>hy?y-hy:hy-y));
    field(b,pos,distance<=4u?" NEAR":distance<=8u?" MID":" FAR");
  } else if(nav&0x80u)
    field(b,pos,(nav&0x10u)?" NEAR":(nav&0x20u)?" FAR":" MID");
}
