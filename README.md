# ANTOPOLIS (GBC)
Populous x SimAnt for Game Boy Color, GBDK-2020. Every graphic (terrain, ants, queens, even the font) is generated in code, so there are no asset files.

## Play
Guide the **black** colony and kill the **red queen**. Lose your own queen and it's over.

| Button | Action |
|---|---|
| D-pad | move cursor (hold to repeat); the 32x32 map scrolls when you near the screen edge |
| A / B | raise / lower land under the cursor (1 mana) |
| SELECT (tap) | flood a 3x3 area (8 mana); fires when you release it |
| SELECT + A | offering: 2 food -> 4 mana |
| SELECT + B | toggle fast forward (4x, shows `X` in the HUD) |
| START | start / restart; in play: **pause + help screen** |

- **Hints:** when an action is refused (no mana, nest, max height, no food...) the bottom HUD row briefly shows why instead of just buzzing.
- **Sandbox:** hold SELECT while pressing START on the title screen: infinite mana, nobody can win or lose. Just terraform and watch the ants.
- **Konami code** (in play): D-pad Up Up Down Down Left Right Left Right, then B, A gives infinite mana and food for that game.
- **Mana workflow:** mana is what you terraform with, and it refills along three paths:
  1. **Trickle**: +1 mana every ~4 s, always on.
  2. **Tribute**: every food your black ants carry home gives +1 mana (so building good foraging routes feeds your terraforming).
  3. **Offering** (SELECT + A): spend 2 food for 4 mana on demand. Food spent this way doesn't hatch ants (3 food each), so it's a trade between a bigger army and more land-shaping. Denied (low buzz) if you have <2 food or full mana. Mana caps at 20.

  ```
  ants forage -> deliver food -> +1 MP and +1 food
                                   |-> 3 food: hatch an ant
                                   '-> SEL+A: 2 food -> 4 MP
  MP -> raise/lower/flood -> bridges & cuts -> better routes -> more food
  ```
- **Populous:** terraform. Ants can't climb cliffs (>1 height step) or cross water, so build bridges for your armies and cut paths for the enemy. Nests can't be edited. Flooding drowns ants.
- **SimAnt:** foragers lay pheromone trails and carry food home. Nests hatch an ant per 3 food, but **only while the queen lives**. Once a colony has 8+ ants, every 4th ant it hatches is a **soldier** that marches on the enemy nest and bites the queen; other ants that reach an enemy nest steal food. Queens have 5 HP and regenerate slowly. Each colony is capped at 17 ants.
- **HUD** (bottom, on the window layer): top row `MP` mana number + 10-cell mana bar (2 mana per cell) and `F` your food; bottom row `Q` your queen HP, `FOE` enemy queen HP, `A` your ants, `R` red ants, `X` at the far right while fast forward is on.
- **Music & sound:** a 4-channel chiptune loop (arpeggio, lead, wave-channel bass and noise drums; A minor, 112 BPM, 8 bars) plays on the title screen and during the game, and keeps playing while paused. Sound effects briefly borrow channels from it: blips for raise/lower/flood, food delivered, hatching, fights and queen hits. The music stops for the short win/lose jingles.

## Build
`make GBDK_HOME=/path/to/gbdk` -> `build/antopolis.gbc` (CGB-only ROM).

## Status
**Not yet built with real GBDK or run on hardware/emulator**: the build environment had no GBDK. The CI workflow (`.github/workflows/build.yml`) builds the ROM and uploads it as an artifact.

Things to check first on a real build:
- HUD/terrain tiles overlapping: terrain tiles start at VRAM tile 128 and the font at 136.
- The window layer (bottom 2 rows) and BG scroll: `LCDC` is set to window map `0x9C00`, BG map `0x9800`.
- Sprite flicker if >10 sprites share a scanline (up to 34 ants + 2 queens + cursor).
- Sound levels/frequencies (calculated, never heard).

## Ideas
Per-colony AI terraforming, more than one queen/nest per side, scoring, save map seed.
