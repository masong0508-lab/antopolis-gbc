# ANTOPOLIS (GBC)
Populous x SimAnt for Game Boy Color, GBDK-2020. Every graphic (terrain, ants, queens, even the font) is generated in code, so there are no asset files.

## Play
Guide the **black** colony and kill the **red queen**. Lose your own queen and it's over.

| Button | Action |
|---|---|
| D-pad | move cursor (hold to repeat); the 32x32 map scrolls when you near the screen edge |
| A / B | raise / lower land under the cursor (1 mana) |
| SELECT (tap) | flood a 3x3 area (8 mana); fires when you release it |
| SELECT + A | embezzle: 2 food -> 4 mana (costs 5 popularity) |
| SELECT + B | toggle fast forward (4x, shows `X` in the HUD) |
| START | start / restart; in play: **pause + help screen** |

- **Hints:** when an action is refused (no mana, nest, max height, no food...) the bottom HUD row briefly shows why instead of just buzzing.
- **Sandbox:** hold SELECT while pressing START on the title screen: infinite mana, nobody can win or lose. Just terraform and watch the ants.
- **Konami code** (in play): D-pad Up Up Down Down Left Right Left Right, then B, A gives infinite mana and food for that game.
- **Mana workflow:** mana is what you terraform with, and it refills along three paths:
  1. **Trickle**: +1 mana every ~4 s, always on.
  2. **Tribute**: every food your black ants carry home gives +1 mana (so building good foraging routes feeds your terraforming).
  3. **Embezzle** (SELECT + A): skim 2 food into 4 mana on demand. That food won't hatch ants (3 food each) and it costs 5 popularity. Denied (low buzz) if you have <2 food or full mana. Mana caps at 20.

  ```
  ants forage -> deliver food -> +1 MP and +1 food
                                   |-> 3 food: hatch an ant
                                   '-> SEL+A: embezzle 2 food -> 4 MP (-5 P)
  MP -> raise/lower/flood -> bridges & cuts -> better routes -> more food
  ```
- **Tropico (El Presidente):** your colony has a **popularity** rating `P` (0-99, starts at 50) that sinks on its own (the people are never satisfied), so you have to keep earning it: food delivered +1, an ant hatched +1; each dead black ant -2 (fights, drowning, your own floods!), your queen bitten -3, embezzling -5, and an empty larder grumbles extra. Every minute or so there is an **election**: P 50+ wins foreign aid (+8 mana), 25-49 gets nothing, under 25 is a **coup** (half your food and all your mana looted, popularity reset to 40). Keep the people fed, and keep your hands out of the till.
- **Populous:** terraform. Ants can't climb cliffs (>1 height step) or cross water, so build bridges for your armies and cut paths for the enemy. Nests can't be edited. Flooding drowns ants.
- **SimAnt:** foragers lay pheromone trails and carry food home. Nests hatch an ant per 3 food, but **only while the queen lives**. Once a colony has 8+ ants, every 4th ant it hatches is a **soldier** that marches on the enemy nest and bites the queen; other ants that reach an enemy nest steal food. Queens have 5 HP and regenerate slowly. Each colony is capped at 17 ants.
- **HUD** (bottom, on the window layer): top row `MP` mana number + 10-cell mana bar (2 mana per cell), `X` while fast forward is on, and `F` your food; bottom row `Q:a/b` your queen HP / enemy queen HP, `A` your ants, `R` red ants, `P` popularity.
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
