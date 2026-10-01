# ANTOPOLIS (GBC)
Populous x SimAnt for Game Boy Color, GBDK-2020. Every graphic (terrain, ants, queens, even the font) is generated in code, so there are no asset files.

## Play
Guide the **black** colony and kill the **red queen**. Lose your own queen and it's over.

| Button | Action |
|---|---|
| D-pad | move cursor (hold to repeat); the 32x32 map scrolls when you near the screen edge |
| A / B | raise / lower land under the cursor (1 mana) |
| SELECT | flood a 3x3 area (8 mana) |
| START | start / restart |

- **Populous:** terraform. Ants can't climb cliffs (>1 height step) or cross water, so build bridges for your armies and cut paths for the enemy. Nests can't be edited. Flooding drowns ants.
- **SimAnt:** foragers lay pheromone trails and carry food home. Nests hatch an ant per 3 food, but **only while the queen lives**. Once a colony has 8+ ants, every 4th ant it hatches is a **soldier** that marches on the enemy nest and bites the queen; other ants that reach an enemy nest steal food. Queens have 5 HP and regenerate slowly. Each colony is capped at 17 ants.
- **HUD** (bottom, on the window layer): `MP` mana, `ANT` your ants, `RED` enemy ants, `QUEEN` your queen HP, `FOE` enemy queen HP, `F` your food.
- **Sound:** blips for raise/lower/flood, food delivered, hatching, fights and queen hits; short win/lose jingles.

## Build
`make GBDK_HOME=/path/to/gbdk` -> `build/antopolis.gbc` (CGB-only ROM).

## Status
**Not yet built with real GBDK or run on hardware/emulator**: the build environment had no GBDK. What *was* done: the game logic was compiled with gcc against hand-written GBDK stubs and run under ASan/UBSan (`tools/hostsim/run.sh`): idle play, random button-mashing and a full-map camera sweep, plus a balance test across many maps (with an idle player the two colonies win about equally often; roughly half of idle games stalemate because terrain blocks the soldiers, which is where terraforming comes in).

Things to check first on a real build:
- HUD/terrain tiles overlapping: terrain tiles start at VRAM tile 128 and the font at 136.
- The window layer (bottom 2 rows) and BG scroll: `LCDC` is set to window map `0x9C00`, BG map `0x9800`.
- Sprite flicker if >10 sprites share a scanline (up to 34 ants + 2 queens + cursor).
- Sound levels/frequencies (calculated, never heard).

## Ideas
Per-colony AI terraforming, more than one queen/nest per side, scoring, save map seed.
