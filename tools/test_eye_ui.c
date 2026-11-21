// Native tests for the exact 20-character ANT EYE HUD copied into window VRAM.
// Keep UI language readable and make every context label fit one GBC row.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define EYE_UI_HOST_TEST 1
#include "../src/eye_ui.c"

static char row[21];
static void expect(const char *want) {
  size_t n = strlen(want), i;
  assert(n <= 20);
  assert(strlen(row) == 20);
  assert(strncmp(row, want, n) == 0);
  for (i = n; i < 20; ++i) assert(row[i] == ' ');
}

int main(void) {
  static const char *const hazards[] = {
    "AHEAD: MAP EDGE", "AHEAD: WATER - STOP", "AHEAD: STEEP CLIFF",
    "AHEAD: CLEAR PATH", "AHEAD: FOOD - WALK", "AHEAD: FRIENDLY ANT",
    "AHEAD: ENEMY - BITE", "AHEAD: HOME NEST", "AHEAD: RED QUEEN"
  };
  static const char *const actions[] = {
    "", "BLOCKED: TURN FIRST", "FOOD PICKED UP", "FOOD DELIVERED",
    "NO ENEMY IN REACH", "ENEMY ANT DEFEATED", "RED QUEEN DAMAGED"
  };
  unsigned i;
  for (i=0; i<9; ++i) {
    eye_ui_ahead(row, (uint8_t)i);
    expect(hazards[i]);
  }
  eye_ui_ahead(row, 255); expect("AHEAD: MAP EDGE");
  for (i=0; i<7; ++i) {
    eye_ui_notice(row, (uint8_t)i);
    expect(actions[i]);
  }
  eye_ui_guide(row, 0, 0, 10, 10, 2, 2);
  expect("NO FOOD NEARBY");
  eye_ui_guide(row, 0x80, 0, 10, 10, 2, 2);
  expect("FOOD: HERE");
  eye_ui_guide(row, 0x80 | 8 | 2, 0, 10, 10, 2, 2);
  expect("FOOD: NORTHWEST");
  eye_ui_guide(row, 0x80 | 4 | 1, 0, 10, 10, 2, 2);
  expect("FOOD: SOUTHEAST");
  eye_ui_guide(row, 0x80 | 4, 0, 10, 10, 2, 2);
  expect("FOOD: SOUTH");
  eye_ui_guide(row, 0, 1, 10, 10, 2, 2);
  expect("HOME: NORTHWEST");
  eye_ui_guide(row, 0, 1, 2, 2, 2, 2);
  expect("HOME: HERE");
  eye_ui_guide(row, 0, 1, 0, 0, 2, 2);
  expect("HOME: SOUTHEAST");
  puts("GBC ANT EYE UI: all actions, objectives and terrain labels fit 20 tiles");
  return 0;
}
