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

## First-person movement bank safety (regression)

Actual-ROM emulator walking tests caught a second runtime arithmetic
failure: `dx *= dir; dy *= dir` in the ROM3 movement helper compiled into
a call to a general multiplication routine in switchable ROM1. Rotate/look
tests did not exercise it. The helper now uses sign inversion for backward
steps; a real-ROM smoke must verify changing the pilot's world coordinates
while LCDC remains enabled, including SELECT pheromone marking.

## Real-world target cue / queen silhouette

In ANT EYE, face water, cliffs, an empty tile, food, a worker, and each
queen (when reachable). The `AHEAD:` indicator must show `W`, `C`,
`O`, `F`, `A`/`R`, and `H`/`Q` respectively; an off-grid direction
shows `X`. It must never claim a bite through a blocked diagonal corner.
Queen posts have a tiny gold cap in the 3D view. The headless emulator now
presses B after a real walk to catch regressions from the banked attack path.

## Re-entering ANT EYE and printable target cues

Emulator CI now checks that the `AHEAD` character is an actual printable
font glyph and that entering ANT EYE again after walking, biting and
returning to the overhead map keeps LCDC active and restores the 2D window
on a second exit. `O` means open terrain, because this font has no period.

## GBC navigation and UI smoke checks

The headless ROM test verifies the title's A tutorial and SELECT sandbox
shortcuts, a printable W/S/G/H terrain glyph beside the 2D HUD queen meter,
the PAUSED overlay, scrollable START HERE guide, and correct return to the
strategy HUD before entering ANT EYE. Copy-text CI also checks every help
line and every tutorial card is at most 20 tiles wide, fits the shared
640-byte scratch buffer, and only uses drawable GBC glyphs.

## 2D/3D terrain correspondence

Compare a height-1 sand tile, height-2 grass tile, height-3 rock hill, and
height-0 water from the overhead strategy map with nearby ANT EYE views.
The scene should show tan soil, green grass, brown rock and blue water
using existing palettes. Near grass can show small tan mottling; distant
grass should remain a solid green. Confirm walking and diagonal blocking
still agree with the 2D map and the AHEAD indicator.

## Foreground ant silhouette

ANT EYE should show two restrained curved dark antennae rising from the
bottom edge (not opaque walls or a HUD sprite). They should stay fixed as
the camera turns, while the world behind them changes. Exiting to the
overhead map must restore its HUD; entering again rebuilds the same masks
without interfering with help text or title scene generation.

## ANT EYE organism silhouette and gradient

Inspect an actual worker, enemy worker, and queen in ANT EYE: a nearby
figure should show small gaps for antennae/legs around a segmented body
rather than a solid vertical rectangle. Ally is dark and enemy red,
with gold on the queens. Distant silhouettes remain compact dots.
The sky should graduate from navy through blue to the light horizon in
the top 24 px of the 3D viewport. The emulator tests three distinct
RGB samples and continues exercising walking, bite and returning to map.
