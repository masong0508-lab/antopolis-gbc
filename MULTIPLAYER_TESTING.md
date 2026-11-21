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
- Carrying food must show correct X/Y HOME arrows in the HUD,
  updating as the ant walks. A cycles living workers without silently
  switching to an unmovable queen camera.
- Entering ANT EYE with no living worker must return to the strategy map
  with an explanation. Re-enter after food and workers change; old occupancy
  sprites must not remain as ghosts in the ray view.
- GBC does **not** yet have simultaneous link-cable ANT EYE or Internet
  multiplayer. Do not advertise those features as supported.

**Current validation:** GitHub build and ROM size checks; cable timing and
real-device play must still be tested on emulators/hardware.
