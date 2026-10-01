# EMPIRE-ANTS (GBC)

*The ruler of you*

An ant-colony terraforming strategy game for Game Boy Color, GBDK-2020. Every graphic (terrain, ants, queens, even the font) is generated in code, so there are no asset files.

## Play
New here? Press **A** on the title screen for a guided tutorial: short lessons, each followed by a hands-on task.

Guide the **black** colony and kill the **red queen**. Lose your own queen and it's over.

| Button | Action |
|---|---|
| D-pad | move cursor (hold to repeat); the 32x32 map scrolls when you near the screen edge |
| A / B | raise / lower land under the cursor (1 mana) |
| SELECT (tap) | flood a 3x3 area (8 mana); fires when you release it |
| SELECT + A | embezzle: 2 food -> 4 mana (costs 5 popularity) |
| SELECT + B | toggle fast forward (4x, shows `FF` in the HUD) |
| START | start / restart; in play: **pause + help screen** (D-pad UP/DOWN scrolls the help text) |

- **Hints:** when an action is refused (no mana, nest, max height, no food...) the bottom HUD row briefly shows why instead of just buzzing.
- **Sandbox:** hold SELECT while pressing START on the title screen: infinite mana, nobody can win or lose. Just terraform and watch the ants.
- **Cheat code** (in play): D-pad Left Left Right Right Up Down Up Down, then B, A gives infinite mana and food for that game.
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
- **Popularity & elections:** your colony has a **popularity** rating `P` (0-99, starts at 50) that sinks on its own (the people are never satisfied), so you have to keep earning it: food delivered +1, an ant hatched +1; each dead black ant -2 (fights, drowning, your own floods!), your queen bitten -3, embezzling -5, and an empty larder grumbles extra. Every minute or so there is an **election**: P 50+ wins foreign aid (+8 mana), 25-49 gets nothing, under 25 is a **coup** (half your food and all your mana looted, popularity reset to 40). Keep the people fed, and keep your hands out of the till.
- **Terraforming:** Ants can't climb cliffs (>1 height step) or cross water, so build bridges for your armies and cut paths for the enemy. Nests can't be edited. Flooding drowns ants.
- **Colony life:** foragers lay pheromone trails and carry food home. Nests hatch an ant per 3 food, but **only while the queen lives**. Once a colony has 8+ ants, every 4th ant it hatches is a **soldier** that marches on the enemy nest and bites the queen; other ants that reach an enemy nest steal food. Queens have 5 HP and regenerate slowly. Each colony is capped at 50 ants (only ants in view get a hardware sprite: 37 at once).
- **HUD** (bottom, on the window layer; labels gold, numbers white): top row `MP` mana number + 10-cell mana bar (2 mana per cell) and `F` your food; bottom row `Q a/b` your queen HP / enemy queen HP, `A` your ants, `R` red ants, `P` popularity, and `FF` while fast forward is on.
- **Pause / help:** START opens a scrolling help list (controls, goal, mana, HUD key, elections, ants) under the HUD; UP/DOWN scrolls, hold to repeat, START resumes.
- **Music & sound:** in the game a 4-channel chiptune loop (arpeggio, lead, wave-channel bass and noise drums; A minor, 112 BPM, 8 bars) keeps playing while paused. Sound effects briefly borrow channels from it: blips for raise/lower/flood, food delivered, hatching, fights and queen hits. The music stops for the short win/lose jingles. The title screen has its own theme (see below).

## Disasters and perks (roguelike)
- **Disasters:** every so often (after the first ~12 s; never in sandbox or the tutorial) a random 3x3 patch of the map is hit, for both colonies alike: a **flash flood** sinks every tile one level, an **earthquake** shakes each tile up or down. Nests are safe. Ants on tiles that hit water drown. They come more often on HARD and less often on EASY.
- **Perks:** after every election that is not a coup, the HUD offers two random perks: `A:xxxxxxx B:xxxxxxx` (about 10 s to choose; press **A** or **B** to take that one, otherwise it is lost). Perks last for the rest of the run and are saved with the game:
  `FLOOD 4` flood costs 4 MP | `FASTEGG` your ants hatch from 2 food | `MANA UP` mana trickles twice as fast | `QUEENUP` +1 queen HP.
- Old saves still load (perks live in unused bits of the save flag byte).

## Levels, saves and records
- **Levels:** on the title screen LEFT/RIGHT picks **EASY / NORMAL / HARD** (remembered). Easy: more starting MP, faster mana, slower popularity loss, tougher queen for you, red hatches slower and its soldiers march later. Hard is the reverse. NORMAL is the original game. The table is `DIF` in `src/main.c`.
- **Save / continue:** while paused, **A** saves the game (A again to overwrite an existing save), **B twice** quits to the title. On the title, **UP** continues the saved game. A finished game uses up its save. Not saved: pheromone trails and fast-forward.
- **Records:** **B** on the title opens per-level wins, losses, fastest win and best score; hold SELECT+B for 2 s there to erase. After a counted game a results card shows time, score and records. Sandbox, tutorial and cheat games do not count.
- **Score (win):** 300/600/900 by level + up to 600 for speed (1 per second under 10 min) + 5 per ant + popularity + 10 per queen HP.
- Battery RAM: the ROM header is MBC5 + RAM + battery (`-Wm-yt0x1B -Wm-ya1 -Wm-yo4` in the Makefile). Untested on hardware, so check that the `.sav` file appears in your emulator.

## Title screen
Night sky with twinkling stars and a crescent moon, the gold 3D logo with a shine that sweeps across it every couple of seconds, rolling hills, and ants marching along the grass (black ones heading right, red ones left). The picture and the music **fade in** when it appears and **fade out** when you press START (or A for the tutorial); the game then fades in too.

The title theme is separate from the in-game loop and **in stereo** (E phrygian-dominant, ~81 BPM, 8 bars):
- **left:** a thin, quiet echo of the lead, one eighth note behind
- **right:** the lead (25% pulse) with a slow vibrato once each note has settled
- **centre:** a plucked 3+3+2 drone bass on the wave channel, with a custom hollow waveform
- **ping-pong:** tiny noise "footsteps" that bounce between left and right

The in-game music and sound effects stay centred (mono), because effects borrow channels from the music.

## Build
`make GBDK_HOME=/path/to/gbdk` -> `build/empire-ants.gbc` (CGB-only ROM).

## Status
**Not yet built with real GBDK or run on hardware/emulator**: the build environment had no GBDK. The CI workflow (`.github/workflows/build.yml`) builds the ROM and uploads it as an artifact.

Things to check first on a real build:
- HUD/terrain tiles overlapping: terrain tiles start at VRAM tile 128 and the font at 136.
- The window layer (bottom 2 rows) and BG scroll: `LCDC` is set to window map `0x9C00`, BG map `0x9800`.
- Sprite flicker if >10 sprites share a scanline (up to 34 ants + 2 queens + cursor).
- Sound levels/frequencies (calculated, never heard). The title theme's stereo split (NR51) only shows on real stereo output or a stereo-capable emulator.
- Title screen: fades step the palettes towards black (8 steps) and write them right after `vsync()`; if you see a flash on a step, that is the place to look (`fade_calc` / `fade_apply`).

## Ideas
Per-colony AI terraforming, more than one queen/nest per side, scoring, save map seed.

## Art data
Terrain, mana-bar, sprite and title-scenery tiles are drawn as readable digit strings in `tools/make_art.py`, which generates `src/art.h` (raw 2bpp tile bytes, about 2 KB smaller than keeping the strings in ROM). Edit the art there and run `python3 tools/make_art.py`.

## Boot logo
The DippInn logo (`src/dippinn_logo*.c/.h`, regenerate with `tools/make_dippinn_logo.py`) plays at power-on; START skips it.
