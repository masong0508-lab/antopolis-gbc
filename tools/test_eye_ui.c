// Native tests for the exact 20-character ANT EYE HUD copied into window VRAM.
// Keep UI language readable and make every context label fit one GBC row.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define EYE_UI_HOST_TEST 1
// The banked HUD reads the live game terrain instead of allocating a map.
uint8_t hgt[32][32];
#include "../src/eye_ui.c"

static char row[21];
static void expect(const char *want) {
  size_t n = strlen(want), i;
  assert(n <= 20);
  // GBC's generated raised font supports letters, numbers, spaces and :!/
  for (i = 0; i < n; ++i) {
    char c = want[i];
    assert(c == ' ' || c == ':' || c == '!' || c == '/' ||
           (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'));
  }
  assert(strlen(row) == 20);
  assert(strncmp(row, want, n) == 0);
  for (i = n; i < 20; ++i) assert(row[i] == ' ');
}

int main(void) {
  // Level terrain makes the home leg legal by default. Water and cliffs
  // are varied below to ensure the HUD does not lead into blocked tiles.
  memset(hgt, 1, sizeof hgt);
  static const char *const hazards[] = {
    "AHEAD: MAP EDGE", "AHEAD: WATER BLOCKS", "AHEAD: STEEP CLIFF",
    "AHEAD: CLEAR PATH", "AHEAD: FOOD WALK", "AHEAD: FRIENDLY ANT",
    "AHEAD: ENEMY BITE", "AHEAD: HOME NEST", "AHEAD: RED QUEEN"
  };
  static const char *const actions[] = {
    "", "BLOCKED: TURN FIRST", "FOOD PICKED UP", "FOOD DELIVERED",
    "NO ENEMY IN REACH", "ENEMY ANT DEFEATED", "RED QUEEN DAMAGED",
    "PRACTICE QUEEN SAFE"
  };
  unsigned i;
  for (i=0; i<9; ++i) {
    eye_ui_ahead(row, (uint8_t)i);
    expect(hazards[i]);
  }
  eye_ui_ahead(row, 255); expect("AHEAD: MAP EDGE");
  for (i=0; i<8; ++i) {
    eye_ui_notice(row, (uint8_t)i);
    expect(actions[i]);
  }
  eye_ui_scent(row, 1); expect("SCENT ON   START MAP");
  eye_ui_scent(row, 0); expect("SEL SCENT  START MAP");
  eye_ui_guide(row, 0, 0, 10, 10, 2, 2);
  expect("NO FOOD NEARBY");
  eye_ui_guide(row, 0x40, 0, 10, 10, 2, 2);
  expect("FOOD: CHECK MAP");
  eye_ui_guide(row, 0x80, 0, 10, 10, 2, 2);
  expect("FOOD: HERE");
  eye_ui_guide(row, 0x80 | 8 | 2, 0, 10, 10, 2, 2);
  expect("FOOD: NORTHWEST MID");
  eye_ui_guide(row, 0x80 | 4 | 1, 0, 10, 10, 2, 2);
  expect("FOOD: SOUTHEAST MID");
  eye_ui_guide(row, 0x80 | 4, 0, 10, 10, 2, 2);
  expect("FOOD: SOUTH MID");
  eye_ui_guide(row, 0x80 | 0x10 | 8 | 2, 0, 10, 10, 2, 2);
  expect("FOOD: NORTHWEST NEAR");
  eye_ui_guide(row, 0x80 | 0x20 | 4 | 1, 0, 10, 10, 2, 2);
  expect("FOOD: SOUTHEAST FAR");
  eye_ui_guide(row, 0, 1, 10, 10, 2, 2);
  expect("HOME: NORTHWEST");
  eye_ui_guide(row, 0, 1, 2, 2, 2, 2);
  expect("HOME: HERE");
  eye_ui_guide(row, 0, 1, 0, 0, 2, 2);
  expect("HOME: SOUTHEAST");
  // Northward ground is water but west is open: do not advertise NW.
  hgt[9][10] = 0;
  eye_ui_guide(row, 0, 1, 10, 10, 2, 2);
  expect("HOME: WEST");
  hgt[10][9] = 0;
  eye_ui_guide(row, 0, 1, 10, 10, 2, 2);
  expect("DETOUR: EAST"); // north is accessible after a safe east step
  // After obeying EAST, the compass must not immediately say WEST
  // and oscillate back into the original dead end. Choose NORTH.
  eye_ui_guide(row, 0, 1, 11, 10, 2, 2);
  expect("HOME: NORTH");
  hgt[9][11] = 0; // east would be a dead end; south then west is open
  eye_ui_guide(row, 0, 1, 10, 10, 2, 2);
  expect("DETOUR: SOUTH");
  hgt[11][9] = 0; // neither bypass has a legal second step
  eye_ui_guide(row, 0, 1, 10, 10, 2, 2);
  expect("HOME: CHECK MAP");
  // Restore the northwest leg, then make the diagonal a steep cliff.
  hgt[9][10] = hgt[10][9] = hgt[9][11] = hgt[11][9] = 1;
  hgt[9][9] = 3;
  eye_ui_guide(row, 0, 1, 10, 10, 2, 2);
  expect("HOME: WEST");
  // Directly north and blocked: no false "HOME: NORTH" bearing.
  hgt[9][10] = 0;
  eye_ui_guide(row, 0, 1, 10, 10, 10, 4);
  expect("DETOUR: EAST");
  // If there is NO two-step local bypass, do not fabricate one.
  hgt[9][11] = hgt[9][9] = hgt[11][11] = hgt[11][9] = 0;
  eye_ui_guide(row, 0, 1, 10, 10, 10, 4);
  expect("HOME: CHECK MAP");
  // A target at the ant's own tile is never a detour.
  eye_ui_guide(row, 0, 1, 10, 10, 10, 10);
  expect("HOME: HERE");
  // A diagonal landing with no onward leg is a misleading home arrow.
  memset(hgt, 1, sizeof hgt);
  hgt[11][12] = hgt[12][11] = 0;
  eye_ui_guide(row, 0, 1, 10, 10, 12, 12);
  expect("HOME: EAST");
  // A diagonal step directly onto home needs no further progress leg.
  eye_ui_guide(row, 0, 1, 10, 10, 11, 11);
  expect("HOME: SOUTHEAST");
  // Mirror the same trap toward the opposite quadrant.
  memset(hgt, 1, sizeof hgt);
  hgt[9][8] = hgt[8][9] = 3;
  eye_ui_guide(row, 0, 1, 10, 10, 8, 8);
  expect("HOME: WEST");
  puts("GBC ANT EYE UI: all actions, objectives and terrain labels fit 20 tiles");
  return 0;
}
