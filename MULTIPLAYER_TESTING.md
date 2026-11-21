# Multiplayer validation: GBC

The standard artifact is **empire-ants.gbc**, produced by GitHub Actions.
Use two **matching current ROM builds**, and a real GBC link cable or an
emulator that supports two linked Game Boy Color consoles.

## Versus link-cable test

1. On both games press DOWN at the title screen for **2 PLAYERS**, then
   choose **LINK CABLE** (not SAME GB).
2. Press START on one console; verify both reach the same generated map,
   with the black player on one and red player on the other.
3. Move the black cursor and raise/lower land. Repeat from the red player.
   Check that both consoles agree on land height, queen HP, ant counts
   and who wins.
4. Open the pause/help screen from either console and resume. Check for
   disjointed turns, stuck input or unresponsive buttons.
5. Disconnect the cable during play. The exchange must time out instead
   of hanging forever, and the game should show LINK LOST.
6. Restart and attempt to link an outdated game build. The revised hello/
   reply handshake (0x5B / 0xA6) should refuse mismatched older builds.

## Single-console and first-person checks

- SAME GB: alternate the controller after each pass card and verify both
  players can edit terrain.
- ANT EYE: single-player only on GBC. Collect food, return it to the nest,
  lay scent with SELECT and bite with B. The colony map must reflect these
  changes when returning with START.
- Carrying food must show correct X HOME arrows and Y N/S directions in the HUD; O
  marks an aligned axis (no blank glyphs),
  updating as the ant walks. A cycles living workers without silently
  switching to an unmovable queen camera.
- Entering ANT EYE with no living worker must return to the strategy map
  with an explanation. Re-enter after food and workers change; old occupancy
  sprites must not remain as ghosts in the ray view.
- Move in all 16 camera directions. Walking should quantize to eight
  cardinal/diagonal steps, including safe backward steps, without crossing
  water or cliffs diagonally through a blocked corner. Camera turns must
  not snap unexpectedly when moving.
- Without carrying food, the F HUD cue should point to the nearest berry
  within eight tiles, or NOFOOD if none exists nearby. Pickup should
  switch to the HOME arrows; after delivery the nearest berry cue should
  reappear. Rotating in place must not force a food scan every frame.
- Bite in all eight facing directions: it must target the cell actually
  ahead in the 3D view, hit enemy ants or the enemy queen when reachable,
  and refuse water/cliff/diagonal-corner attacks. Biting must not move the ant.
- Enter ANT EYE after viewing a menu or pause screen: the upper two window
  rows must not retain stale old-map/help text. "ANT EYE" labels the view.
  Hill crests should show a 1-pixel dark ridge; distant grass should be
  subdued without affecting water or close-up food and ants.
- Run `make test` with a normal C compiler to exercise movement, corner
  collisions, compass bearings, and depth-based terrain classes.
- GBC does **not** yet have simultaneous link-cable ANT EYE or Internet
  multiplayer. Do not advertise those features as supported.

**Current validation:** GitHub build and ROM size checks; cable timing and
real-device play must still be tested on emulators/hardware.

## Automated actual-ROM GBC emulator smoke

The headless PyBoy test (`tools/gbc_eye_smoke.py`) explicitly skips the
DippInn boot-logo gate before issuing the separate START needed to launch a
game. It checks the strategy HUD's M glyph so startup screens cannot be
mistaken for ANT EYE gameplay. It runs against the
**built .gbc ROM** in CI. It verifies the title, the live strategy map,
SELECT+START entering ANT EYE, the dedicated 3D VRAM/window layout,
a visible camera turn, and return to the 2D map. It uploads actual emulator
screenshots and a JSON report. An emulator boot smoke is not a substitute
for human testing of terrain collisions, long sessions, audio or physical
GBC link-cable timing.

## Fixed-bank trampoline safety

The old monolithic linker layout put GBDK's `__sdcc_bcall_ehl` trampoline
in switchable ROM1 at about 0x7DE8. Any ANT EYE function called with
`BANKED` selected ROM3 and immediately switched the executing trampoline
out, causing an emulator LCD shutdown. The Makefile now reserves a fixed
ROM0 slot at 0x0200 for `_HOME` before linking `_CODE` at 0x0500;
CI inspects the actual symbol address and fails builds if `_HOME`
moves into switchable space. This must still pass a real ANT EYE emulator
screen-transition test, not just a bank-space assertion.

## ANT EYE bank3 arithmetic boundary

Ray projection must never invoke an unbanked compiler arithmetic helper
that the linker has placed in the **switchable** part of ROM1. In particular,
the ray angle multiplied by a variable column offset used to compile into
a call to `__mulint` in ROM1. Under bank3 that address resolves to unrelated
instructions, causing LCD shutdown. The ray now computes these tiny products
with a bank-local shift/add routine. Keep the native signed-multiplication
regression test and the full actual-ROM emulator smoke in CI.

## ANT EYE pheromone/worker movement smoke

The actual-ROM emulator test now searches reachable facings and asserts
that walking changes the selected worker's displayed world coordinates,
while holding SELECT to leave real pheromone. It then verifies the window
returns to the overhead map. The 3D ray marcher displays strong local
pheromone on height 1-2 soil as a small gold marker, using palette 6.
Native tests check the exact soil, strength, center and distance thresholds.
