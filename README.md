## Map shortcuts: jump to your queen or the rival nest (GBC)

In the overhead map, **hold SELECT and press UP** to recenter the cursor
on your own queen's nest. **Hold SELECT and press DOWN** to jump to
the rival nest. These controls work for either player's colony, are
shown in the in-game pause help and cursor tutorial, and cost no mana.
Holding either chord does not slide the cursor away from the target.

Releasing SELECT after a nest shortcut **does not trigger a flood**.
Normal tap-SELECT flooding, SELECT+A food trading, SELECT+B fast
forward and SELECT+START ANT EYE remain unchanged. Only the existing
cursor coordinates are updated; workers, queen health, map terrain,
battery-save layout, and link-cable protocol remain as before.

For ordinary D-pad scrolling, the usual immediate movement, 10-frame
delay and 3-frame repeats remain unchanged on normal short holds.
The old 250-frame cap caused the repeat rate to jump to every frame
during very long holds; the counter now wraps instead of saturating.
The shortcut reuses the existing cursor and SELECT-consumed bit, with
no extra world buffer. The native test and headless GBC ROM test cover
both nest jumps, normal hold-to-repeat movement and terrain integrity.

## Colony-growth awards and better ANT EYE foraging (GBC)

In normal **single-player** games, hatching the 8th, 16th and 24th
worker earns a one-time **+3, +4 and +5 mana** bonus, respectively
(up to the usual 20-mana cap). A HUD message announces each milestone,
providing a visible reason to grow the colony. The already-saved
hatch counter determines the reward: there are no new SRAM bytes,
per-frame timers or temporary progress state. The bonus is deliberately
disabled in tutorials, sandbox and multiplayer, preserving existing
practice expectations and link-cable rules.

In ANT EYE, the food compass checks a **second legal step** before
proposing a cardinal direction. This helps avoid returning immediately
to a blocked position and, for a direct berry with an obstacle one
tile ahead, can offer a verified two-step sidestep. It is **local
guidance within the existing eight-tile scan**, not full pathfinding.
The updated pause help and ANT EYE lesson explain the food guide.
No new RAM buffers, ray casts or per-frame scans were introduced.

## Easier starting and faster manual navigation (GBC)

The GBC title now explicitly identifies **START: PLAY**, separate from
**A: TUTORIAL**, and explains the immediate goal:
**GOAL: FEED YOUR ANTS**. Other actions—difficulty arrows, saved
game Continue, local/link multiplayer and SELECT+START sandbox—are
unchanged.

Pause with START to open the existing command manual. UP/DOWN still
scroll **one line** at a time; LEFT/RIGHT now jump **13 lines**,
one complete visible page. Both directions clamp safely at the top and
bottom. The 20-character control footer displays the page shortcut
and START resume action.

A strict host test checks **327,680** combinations of help length,
position and navigation direction. Source integration checks protect
the exact title and pause controls, and an actual PyBoy cartridge test
reads their rendered text, visits the next page and back, then resumes
the same colony. No new game RAM, saved fields, renderer or per-frame
logic is added.

## ANT EYE movement and bite responsiveness: incremental occupancy

The former GBC first-person input path cleared **all 1,024 bytes** of
its occupancy map, rescanned **all 1,024 food cells** and all 50 ants,
then reinserted both queens **every time the controlled ant moved,
switched, picked up food, or bit an enemy**. This added needless latency
on hardware with an especially slow 3D software ray marcher.

ANT EYE now builds its complete occupancy atlas **only on entry**.
Subsequent events refresh only the old and new worker tiles (one tile
for a bite or stationary food event) using the same strict priority:
berry, then living ants in slot order, then queens. The local update
runs in spacious ROM bank 3 with audio interrupts guarded, not in the
nearly full fixed ROM. It correctly restores a berry or second ant
hidden underneath a moving worker and removes dead enemy occupancy
immediately.

There is **no new 2D or 3D buffer, heap, per-frame update or save
change**. The existing 1,024-byte occupancy map and 300-byte worker
array are shared without resizing. Relative to the old path, each
input-event refresh avoids two full 32×32 memory walks and visits at
most 50 workers for each changed tile.

The native differential regression compares **8,000** movement,
kill/revival, food, overlap and queen transitions against a full-world
reference, checking every one of the **1,024 world cells** after each
action. An integration gate ensures neither movement nor bites can
silently restore the expensive full rebuild. Final cartridge, bank
layout, input/food-save, queen-safety, palette and actual PyBoy 3D
emulator checks remain mandatory before merging.

## Practice queen damage and honest ANT EYE feedback

Sandbox and tutorials already protect the last heart of each queen.
However, ordinary overhead soldier attacks still deducted popularity
and played queen-hit feedback even when the damage was blocked.
Likewise ANT EYE described a blocked last-heart bite as "RED QUEEN
DAMAGED," making practice combat look like a victory attempt.

Damage, popularity and hit effects now occur only when a queen
actually loses a heart. A protected last-heart bite instead displays
**PRACTICE QUEEN SAFE**. The original victory and result flow remains
unchanged in normal games, including regular queen knockouts.

This changes no battery-save fields, RAM allocations, frame buffers,
terrain maps, movement rules or random draws. Native UI tests check
all eight action responses at the actual 20-character GBC width.
The real-ROM PyBoy queen-safety suite now requires the protection
message after repeated bites, plus surviving sandbox games, normal
wins, and correct save checksums. Fixed ROM bank limits remain gated.

## Battery-save integrity: corrupted positions cannot enter live games

The previous GBC loader trusted the save checksum alone. An intact
checksum cannot guarantee valid gameplay data: a saved cursor or living
ant with an x/y coordinate beyond the 32×32 world could be offered as a
continue slot and then index outside the terrain arrays.

Continue now rejects out-of-range live cursor/ant coordinates and
unsupported difficulty values **before loading**. Unused ant slots may
still contain stale coordinates, preserving compatibility with
historical original-format battery saves. Missing, invalid and
corrupted games simply do not offer Continue; per-difficulty records
remain independent and unaffected.

The existing checksum, save record layout, terrain food normalization,
RAM footprint and regular gameplay loop are unchanged. Validation runs
only during title/save loading. CI runs the real PyBoy ROM with a
correct-checksum save containing invalid cursor X/Y, invalid live ant
X/Y, invalid difficulty, a valid original save, and a valid save with
stale inactive-ant coordinates. It checks that legitimate Continue is
preserved and malformed games are never offered as playable.

## ANT EYE worker selection and action feedback

Switching workers now updates the view position before processing any
movement, bite or scent input from that same frame. Previously A plus a
direction could move the newly selected worker beside the old worker,
even across disconnected islands. Simultaneous actions now originate
from the worker actually selected.

Submerged workers are excluded from direct control, and first-person
navigation rejects a water origin. A worker awaiting its ordinary colony
drowning update cannot be rescued by entering ANT EYE and walking onto
nearby dry land. If no eligible worker remains, the game stays on the map
and displays its existing NO WORKER: GROW COLONY message.

Laying scent while stationary now produces immediate on-screen feedback.
The existing view is refreshed when a tile gains scent, and the footer
responds to pressing or releasing SELECT. There is no continuous extra
ray sweep while holding the button over the same marked tile.

An actual-ROM regression uses valid original-format battery fixtures,
Continue and ordinary controls to check separated workers, simultaneous
selection/actions, submerged workers and scent feedback. Existing save,
queen-safety, palette and graphics checks remain release gates.
The cartridge remains 64 KiB with 6,158 bytes of permanent WRAM. This
pass uses 111 bytes of the recovered fixed-code space, leaving 265 bytes;
the release workflow continues to enforce the bank and trampoline limits.

## More fixed-bank space and faster fades

Palette tables and fade arithmetic now live together in ROM bank 3,
freeing 373 bytes in the crowded fixed code area. Its remaining space
increased from 3 to 376 bytes while the cartridge stays 64 KiB and
permanent WRAM remains 6,158 bytes. Four-color batches fill the existing
background and sprite buffers; no new framebuffer or palette RAM is used.

The fade calculation uses exact shift/add arithmetic and avoids calling
library multiplication code through the switched-out bank. Each banked
batch is guarded, and interrupts can run again before the next palette.
All 92 authored colors, nine brightness levels, palette upload counts,
and explicit fade waits are preserved.

Native tests compare all 589,824 encoded color/brightness combinations,
the original palette fingerprint, and every bounded four-color batch.
Actual PyBoy palette reads also verify title/game colors. In the matched
transition comparison, full gameplay color appeared 205 frames after
START instead of 228; title black appeared at frame 42 instead of 53.
This reduces transition overhead; it is not a gameplay frame-rate claim.
The existing ANT EYE and five queen-safety/mode-transition checks remain
release gates, along with the fixed-code and trampoline bounds.

## Sandbox and tutorial queen protection

Practice games can no longer end because a queen dies in ANT EYE or
during a crowded colony attack. The ordinary map healed practice queens
before simulation, but several soldiers could remove all their hearts
in the following slice. ANT EYE bypassed that healing entirely. Both
queen-damage paths now preserve the final heart in sandbox or an active
tutorial lesson; ordinary games still allow the queen to fall.
Starting or continuing another match also resets the active lesson state
from the title's mode selection, so quitting a tutorial cannot carry its
protection into an ordinary game.

The PyBoy regression loads original-format battery saves through the
game's Continue command, then uses real controls to bite the red queen
in ANT EYE or lets a group of soldiers attack her in the overhead view.
Both sandbox cases must stay playable, and both normal-mode controls
must reach the win screen. The previous release fails the two sandbox
cases. Tests inspect the shipping ROM without changing runtime game
state. A fifth case starts a tutorial, quits a live lesson, continues an
ordinary save and requires a normal victory. The save format and checksum
are unchanged. CI also checks the fixed code's exact end address against
the 32 KiB mapping, in addition to the banked-call trampoline bounds.

## Milestone: flood-safe food and honest terrain feedback (November 2026)

GBC terrain edits and natural disasters now remove berries when a cell
becomes water, matching the GBA port. Raising land again does **not**
resurrect drowned food. Classic workers, first-person ANT EYE, and the
3D occupancy view all ignore food on submerged tiles; loading an older
battery-save normalizes submerged food while leaving the save structure
and existing ant/terrain state intact.

Lowering a berry-bearing sand cell reports **FOOD WASHED AWAY**.
Player floods report that terrain changed, and erase submerged berries
without an extra scan. These changes use the current nine-cell event pass, preserve
existing earthquake RNG order and multiplayer rules, and require no
new permanent RAM, framebuffer, or extra ray casts.

A native regression exercises every terrain height, food state,
sinking transition, and old-save normalization. To maintain fixed-bank
ROM headroom, the once-per-match win/loss/fastest-score record update now
executes from switchable ROM bank 3. The existing 18-byte battery-save
record, checksum, update thresholds and result flags are unchanged;
native tests exercise three difficulty slots, saturation at 99 and the
same fastest/score flags. There is no additional simulation-frame cost.

Full ROM build and
emulator checks remain required before release.

## Graphics restoration — GBC terrain, anatomy and role colors (November 2026)

The overhead colony now uses a coherent pixel-art treatment rather than
flat monochromatic cells. All **six base terrain tiles** have been redrawn:
blue water with short ripples, warm sand with subtle pebbles, distinct mossy
grass, stratified brown cliffs, a concentric queen burrow with a dark
entrance, and readable gold berry clusters. The **eight existing terrain
detail variations** add waves, vegetation and varied rock textures without
adding any BG tiles or static map arrays.

Workers, queens and both walking frames were repainted around visible
head, thorax, abdomen, antennae and legs. The two carrying poses have
golden berries; queens retain their crown silhouette. **Seven existing
OBJ sprite tiles** remain the complete overhead animation atlas.

Game Boy Color has eight independent OBJ palettes, so sprites now use
seven of them without adding OAM entries: base black/red workers,
high-contrast cursor, gold-carrying workers of either team, and
steel/ember-armored soldiers. Palette selection reuses the existing
per-worker OAM property write and the normal whole-screen fade, so colors
remain coherent during screen transitions. The same 240-tile ANT EYE
raymarcher uses stronger head/thorax/abdomen and angled-leg masks,
not additional rays, sprites, framebuffers, or per-frame geometry passes.

Artwork tests reconstruct **all base terrain, detail and sprite tiles**
from their source pixel rows and compare every encoded byte with the
shipping GBC data. Native tests also verify role palette assignment;
the full PyBoy test must still pass to establish on-device playability.
All role/gameplay inputs, collision, RNG and save data remain unchanged.

### Cohesive 3D terrain instead of checkerboard grass

ANT EYE previously colored entire nearby grass *cells* alternately green
and sandy tan using an `(x+y)` checkerboard. This made the first-person
world resemble a patchwork grid rather than continuous vegetation. Grass
now has one consistent ground color at all depths; existing per-pixel
tufts still create surface detail inside its tiles. Water, sand and rock
retain distinct material classes. The bank-3 color test now checks
grass consistency across every world cell and near/far depths. This
removes a branch from the ray material picker, requires no new images
or buffers and leaves the scarce fixed ROM bank untouched.

### Narrow, non-occluding ANT EYE limbs

Peripheral view rays no longer paint a repeated eight-pixel-wide ant
pattern for the full height of the animal. Instead they draw two sparse
leg strokes and leave the underlying landscape visible; the central
ray columns form the connected creature. This removes the checkerboard
fence visible in previous enlarged emulator captures, without new
VRAM tiles, RAM, assets or additional ray casts. Queen size, ant
ownership, collision and combat remain unchanged.

### One organism per ray hit rather than a repeated sprite fence

ANT EYE now distinguishes the creature's **central body** from its
projected outline: only rays intersecting the narrow centre paint the
connected head/thorax/abdomen, while outer rays contribute sparse legs.
Previously each of several adjacent columns rendered the entire animal,
creating the repeated checkerboard seen in the GBC screenshot. Food also
uses the narrower centre silhouette instead of a wide yellow block.
These are pure bank-3 ray rules plus a same-size 8-byte leg mask. No extra
RAM, VRAM tiles, sprites, or frame passes are required. Host regression
tests verify the central-versus-outer bounds.

### In-world first-person movement validation

The main-map cursor now tracks the actual selected ant during ANT EYE
rather than only updating after mode exit. This reuses the existing two
WRAM bytes (no new allocation). The emulator reads their linker-defined
addresses from the real ROM's map file to verify movement, whereas older
smoke tests incorrectly parsed X/Y letters that were removed in the new
goal-oriented HUD. It also checks readable FOOD/HOME messages after
turning and the current combat result words, avoiding regressions caused
by evolving game text.

### Art provenance and sprite consistency

The GitHub native-test job now checks that all five 8×8 source sprite sketches
exactly match the 80 committed Game Boy tile bytes, that queens are distinct
from workers, and that the two walking frames actually differ. This catches
stale generated art before shipping a ROM, with no runtime code or memory cost.

### ANT EYE organic creature silhouette

3D ants now use connected filled head/thorax/abdomen pixels and sparse
six-legged outlines, instead of the repeated 0xA5 checkerboard pattern.
The replacement uses the same 16 ROM bytes and adds no RAM or extra
rays. This preserves the shorter two-column rendering slices that
improve controller responsiveness.

### ANT EYE controller latency

The 3D viewport now draws **two columns per input cycle** instead of four. This bounds each expensive software-raster slice, allowing more responsive movement, turning, and bite input on the real GBC CPU. The image still renders the same 20 columns and 12 rows, using the same tiles and no extra frame buffers. The final scene may take more shorter steps to refresh, so CI now retains a real-emulator short-button combat regression instead of relying exclusively on source-level tests.

### More recognizable map ants

The overhead strategy view's real 8×8 worker sprite now has visible antennae, head, thorax, abdomen and six legs; the two title-screen walking frames use the same silhouette with alternating legs instead of unrelated old artwork. The queen gets a larger abdomen and a bright crown/antennae distinction. The design reuses the existing five 2bpp sprite tiles and the original three CGB palettes: no new OAM allocations or per-frame sprite drawing cost. Both the source pixel patterns and the generated cartridge art have been updated.

### GBC ANT EYE creature proportions

First-person ants now have a narrower centre-of-cell render footprint, a
21-pixel near-worker height cap and a larger 30-pixel queen cap. Rather
than stretching the same 8px leg texture into a broad checkerboard wall,
the renderer paints just a few anatomically distinct rows for antennae
and legs around a solid team-colored body. Food objects remain compact.
The underlying ant locations, collision, attacks and scent system are
unchanged; no additional RAM, sprite objects or framebuffer allocations
are made. The pure visibility and silhouette-size rules are native-tested.

### Path-aware first-person food directions

Food navigation now checks the ant's actual first step before showing a direction. A diagonal corner blocked by water or a cliff will yield a reachable cardinal alternative, and a nearby food target that cannot be approached from the current tile is skipped for a farther viable one. This eliminates misleading immediate "walk into water" arrows without adding a BFS queue, a new world buffer, or per-frame processing. The existing eight-tile scan still runs only after a worker walks or is selected; native tests cover diagonal obstructions, alternative food and fully blocked approaches.

### Visible food-carrying workers

The 2D colony now distinguishes a worker returning with food from one
searching empty-handed. Two newly drawn eight-pixel carrying poses put a
contrasting food packet above the ant's moving body. They occupy only
two extra **16-byte 2bpp sprite tiles** on the cartridge. The existing
black/red sprite palettes and hardware OAM positions are preserved.

The map sprite pass reuses the **first 36 bytes of the game's existing
640-byte title/help scratch buffer** for the 36 visible ants' carry
flags. The buffer is idle throughout ordinary overhead-map gameplay.
This avoids the costly variable-bit-shift code of an earlier packed-mask
implementation and restores fixed ROM headroom. The correct empty or
carrying tile is selected together in ROM bank 3 every fourth frame.
No additional permanent WRAM, per-frame scan, framebuffer, OAM
allocation or world-tick change is required. Native tests check every
worker index, both walking phases, array boundaries and 256 combinations
of carriers; artwork is compared byte-for-byte with the generated
2bpp tiles.

### GBC fixed-ROM headroom and actual colony animation

The hardware can draw only ten sprites per scanline. The black and
red queens plus the two player cursors now take the **first four OAM
slots**; the 36 animated colony workers take slots 4–39. This prevents
a crowd of workers from hiding the essential queen or selection marker.
Every return from the animated title restores those reserved sprites'
proper queen/cursor silhouettes. The emulator test checks actual OAM
indices and that workers' two-frame animation alternates in gameplay.

The GBC linker caught the fixed game-code bank spilling past `0x7FFF`
after the new overhead colony animation. To protect real hardware rather
than shipping a cartridge with bank overlap, the **one-time** hill-bump
and cliff-smoothing functions have been moved into switchable ROM bank 3.
Both run only while a new world is generated, so there is no new
simulation-frame cost. Calls guard the bank-1 music interrupt. The
original smoothing logic and the water/land classification are preserved.

Native regression tests exercise 50 deterministic random terrain maps,
edge-of-world hill bumps, and water-preserving smoothing. The actual
visible walking ants share one guarded bank-3 shadow-OAM animation pass
every fourth frame, reusing the cartridge's existing two silhouette
tiles. No additional framebuffer, save format change or OAM slot is
needed. The build workflow continues to fail closed if the fixed code
bank overflows or the BANKED trampoline reaches switchable ROM.

### ANT EYE short-range home detours

When carrying food, the first-person compass can now suggest a safe
`DETOUR: NORTH/EAST/SOUTH/WEST` if the direct homeward step is
blocked by water or a cliff. It checks the actual terrain for **two
legal cardinal legs** and rejects a second move that merely backtracks
to the original tile. This helps a worker steer around small moats
instead of always saying `HOME: CHECK MAP` despite an open bypass.
It deliberately does **not** claim a full maze route: when no local
two-step detour exists the prompt still says to consult the map.
The check runs on ant movement/selection, not on 3D ray frames, and
requires no new RAM navigation map, heap or sprite resources. Native
tests cover east/south fallback and a genuinely closed corridor.

### Safer first-person return-home guidance

When carrying a berry, ANT EYE now checks the next actual terrain step
before suggesting a direction home. Water, cliffs and blocked diagonal
corners no longer generate a misleading compass instruction: the HUD
chooses an open cardinal alternative where possible or shows
`HOME: CHECK MAP` when the direct approach is blocked. This is an honest
**one-step safety check**, not a claim that a complete route through the
maze has been found. It reuses the existing world-height array, performs
no per-frame scan, and adds no VRAM/framebuffer or RAM map allocation.
Native tests cover water, diagonal cliffs and fully blocked approaches.

### ANT EYE action feedback

The first-person command panel now reports **BLOCKED: TURN FIRST**, **FOOD PICKED UP**, **FOOD DELIVERED**, **NO ENEMY IN REACH**, **ENEMY ANT DEFEATED**, or **RED QUEEN DAMAGED** after real gameplay actions. These are actual movement, inventory and hit results, not decorative guesses; readable **FOOD: NORTHWEST** and **HOME: SOUTHEAST** directions replace raw X/Y diagnostics when the player moves or turns. Invalid bites and blocked steps avoid unnecessary ray-render passes. Banked text formatting reuses the existing 640-byte scratch and does not allocate additional VRAM buffers.

### Explicit direction

ANT EYE's upper status bar now reads **ANT EYE ANT05 DIR NE** (with live ant number and compass) and its second line spells out **AHEAD: WATER / ENEMY / OPEN**. This removes the ambiguous trailing slash-and-letter from the previous target display, and clears old two-letter headings when turning to a one-letter cardinal direction.

### Strategy HUD readability

The overhead colony now labels its top resources **MANA** and **FOOD** in words, with a five-step mana indicator that fits beside both numbers. Holding SELECT replaces the cryptic stat strip with the controls it modifies: **A:TRADE B:FAST START**; releasing SELECT restores the colony status counters. New games give the clear initial instruction **A RAISE B LOWER**. These changes preserve the two-row gameplay window and do not reduce the visible map.

### ANT EYE HUD redesign

First-person mode uses a distinct **ANT EYE** header, human-readable look-ahead warnings such as **AHEAD: WATER BLOCKS** and **AHEAD: ENEMY BITE**, and separate command rows. Food/home guidance now spells out whole directions instead of displaying cryptic compass bytes and X/Y debug values. ANT EYE resolves pickups or deliveries immediately when switching to a worker already on food or at the nest. The formatter uses scratch bytes 480-500, updates on player actions, and is covered by native 20-column glyph/label tests and emulator text checks.

# EMPIRE-ANTS (GBC)

*The ruler of you*

An ant-colony terraforming strategy game for Game Boy Color, GBDK-2020. Every graphic (terrain, ants, queens, even the font) is generated in code, so there are no asset files.

## Gameplay and multiplayer update (November 2026)

- **Sky color continuity:** the first palette restoration test exposed a bright yellow strip in the 3D viewport. The upper sky now fades from deep indigo through blue to pale cyan with the horizon following the same hue family; the emulator checks that all three sky rows are blue-dominant and distinctly shaded.


- **Critical CGB palette restoration fix:** the game used eight background palettes but the title-to-game fade restored only the first six. Palette 6 (gold scent) and palette 7 (atmospheric sky) silently retained title-screen colors, causing the black sky and inconsistent highlights. Both title and gameplay fades now update all eight CGB palette slots. The emulator checks the actual three distinct blue sky bands in normal gameplay and the banked no-food diagnostic.


- **Fixed-bank capacity recovery:** perspective height/depth lookup now uses a 175-byte static ROM3 table, copied once at boot in place of the original nested 16-bit divide loops in the nearly full base ROM. The table preserves the original signed, clamped offsets exactly. This allows more renderer/UI detail without overflowing the 32KB always-mapped ROM and prevents banked division-helper crashes.


- **Readable 3D ants and twilight atmosphere:** real worker and queen ray hits now render with segmented 8-pixel body and leg/antenna masks, rather than solid vertical rectangles. Red/black ownership and queen gold crown remain intact. The upper ANT EYE sky uses three graded blue CGB shades and a luminous horizon. No new sprite objects, ray casts, framebuffers, or RAM buffers; the same 240 tile-column renderer is used, with additional emulator sky checks.


- **Immersive ANT EYE foreground:** two curved, dark ant antennae and a narrow mandible/head ridge frame the lower first-person view, giving the pilot a visible body reference. The mask is generated once in bank 3 into 480 bytes of the existing text scratch buffer and composited into normal ray-column tile uploads. This costs no new RAM allocation, OAM sprites, or extra ray casts.


- **ANT EYE atmosphere and material texture:** the 3D viewport now has a separate indigo upper-sky palette and fine, deterministic one-pixel stippling for sand, grass, rock, and water. Terrain rays and depth geometry remain unchanged; existing tile-column uploads still handle the drawing, with no extra rays, framebuffers, or OAM sprites. Gold scent and combat sprites remain untextured for clarity.


- **Full-screen command center:** the pause/help guide now clears the gameplay HUD and presents its own brass header and divider. Resume immediately repaints the correct strategy HUD, with explicit emulator assertions for both transitions; no stale map labels bleed through.


- **More detailed colony map:** water, soil, grass, and hill terrain now use two extra hand-crafted 8x8 detail variants each, creating visible currents, sand cracks, grass tufts, and rock seams. Terrain variation is coordinate-stable across camera/mode switches and uses eight spare VRAM tiles with no RAM map, no per-frame redraw, and no changes to gameplay terrain heights.


- **GBC visual UI revitalization:** a banked 5×7 shadowed typeface replaces the old thin pixel lettering throughout the title, HUD, lessons and menus. The title uses a distinct midnight-blue control panel, bronze rule and selected difficulty lighting; pause, records, tutorial and versus receive deep navy backgrounds and warm gold heading bars. Artwork is composed only at boot in ROM3; no new per-frame arrays or framebuffers.


- **True terrain colours in ANT EYE:** first-person sand now renders warm
  tan, living grass green, cliffs dark brown, and water blue, matching the
  actual height classes visible on the overhead colony map. Far grass loses
  noisy checkerboarding. This reuses existing 2bpp palettes; it adds no
  texture data or framebuffer, and is covered by native terrain tests.
- **UI readability pass:** the title screen now distinguishes START PLAY,
  A TUTORIAL, DOWN VERSUS, and continuing a save. The pause guide opens
  with immediate goals and controls, then shows ANT EYE, HUD, and rules
  in clear sections. The overhead HUD has a tiny live cursor-terrain cue:
  W water, S sand, G grass, H hill. Its redraw is change-triggered, not
  an added per-frame overlay. Text-width/font-validation CI prevents broken
  GBC labels and overwriting the 640-byte text buffer.


- **2 players:** on the title screen press DOWN, choose SAME GB (30-second turns) or LINK CABLE (two GBCs). The serial exchange has a finite timeout, and the multiplayer greeting uses a revised version token so older incompatible ROMs are rejected. **Both GBCs must run the newest matching game build.** This is serial-cable versus, not Internet multiplayer.
- **Playable ANT EYE:** in single-player gameplay hold SELECT and press START to inhabit the nearest living black worker. If none exist, the game explains that a worker must hatch before entering. A switches worker, L/R or LEFT/RIGHT turn, UP/DOWN walk, B bites an enemy directly ahead, and hold SELECT to leave a worker-followed scent trail. Walk onto food to pick it up; the ANT EYE HUD now shows east/west arrows and N/S letters toward the home nest while carrying food (O means aligned). Return to its centre to deliver food and mana. Worker switching skips the queen camera; entering ANT EYE clears stale occupancy markers. START returns to the strategy map at the ant's actual position.
- **Palette count safety:** the GBC's seven background palettes now occupy
  all 28 expected colour entries. This fixes a silent truncation of the
  green-and-gold pheromone palette and adds both a compile-time length guard
  and a CI check for accidentally truncated graphics tables.
- **Look-ahead target HUD:** ANT EYE now shows a compact real-world
  cell category when you face a nearby object: `X` map boundary, `W` water,
  `C` cliff/corner, `O` open ground, `F` food, `A` allied worker,
  `R` enemy worker, `H` home queen, `Q` rival queen. This shares the
  eight-direction collision test with walking and biting; it refreshes on
  movement and turns rather than every frame. Queens in 3D also have a
  two-pixel gold crown to distinguish them from regular workers.
- **World-linked 3D pheromone trails:** ANT EYE now reveals nearby strong
  scent as small golden surface markers on actual walkable terrain. Holding
  SELECT while walking leaves a pheromone trail that is shared with the
  overhead colony simulation. Rendering uses a dedicated green-and-gold
  GBC palette, existing world data, and no new framebuffer or sprite sheet.
  Native visibility tests and emulator movement testing cover the feature.
- **GBC bank-safe 3D math:** the ray marcher no longer calls the compiler's
  general-purpose 16-bit multiply helper, which was placed in switchable
  ROM1 and became unavailable when bank3 was active. Its small angle/column
  products now use an in-bank shift/add routine, with native tests over
  positive and negative ray offsets. Combined with the pinned ROM0 bank-call
  trampoline, this prevents two independent ways of crashing on entry to
  ANT EYE. CI includes real-ROM emulator verification of the transition.
- **Improved first-person presentation:** entering ANT EYE now clears stale
  window tiles at the top of the picture and labels the view clearly. Near
  ridgelines have a 1-pixel brown relief edge and distant grass is rendered
  with a quieter darker shade, making the landscape less visually flat
  without adding textures or framebuffers. Rendering remains in bank 3.
- **GBC ANT EYE controls and guidance:** first-person movement now uses eight
  movement directions (the camera still rotates through sixteen angles).
  A diagonal step checks both adjacent cells and the destination, avoiding
  unrealistic corner-cutting through water or cliffs. When not carrying
  food, the HUD shows plain-language hints such as `FOOD: NORTHWEST`
  toward a nearby berry within eight tiles; `NO FOOD NEARBY` means none
  is within that scan radius. When carrying food the HUD instead gives
  `HOME: SOUTH`-style directions toward the black nest.
  The bearing is a proximity hint, not pathfinding around obstacles; use
  the overhead map and terraform blocked paths. The food search runs only
  after changing workers or tiles, not during every camera frame. The
  movement and navigation helpers are kept in the spacious switchable
  ROM bank 3 and have independent native regression tests.
- **ANT EYE bite alignment:** attacks now target the actual forward-facing
  neighboring tile (including diagonals), using the exact same water,
  cliff-height, map-edge and corner-clearance rules as walking. The calculation
  runs in switchable ROM bank 3 rather than adding more code to the nearly full
  fixed bank. It keeps old food, queen, and enemy interactions intact.
- **World continuity:** first-person food, enemies, queen damage, movement and scent trails now use the same tile-state and colony data as the strategy view. The wider colony simulation remains paused while the low-power GBC draws ANT EYE. The ray-marching calculations live in switchable ROM bank 3; actual VRAM drawing remains in the main bank, and interrupts are masked briefly during each ray calculation to protect audio code. Holding a movement/turn direction now allows every column of a ray pass to finish before another full pass starts, preventing stale portions of the 3D picture.
- **Current multiplayer scope:** GBC LINK CABLE multiplayer is versus on the strategy map. First-person and Internet-synchronized multiplayer are not yet supported for two connected GBCs. Build success is not proof of real-hardware link reliability; validate with two linked emulators or physical consoles.

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
- **Colony life:** foragers lay pheromone trails and carry food home. Nests hatch an ant per 3 food, but **only while the queen lives**. Once a colony has 8+ ants, every 4th ant it hatches is a **soldier** that marches on the enemy nest and bites the queen; other ants that reach an enemy nest steal food. Queens have 5 HP and regenerate slowly. Each colony is capped at 25 ants (only ants in view get a hardware sprite: 37 at once).
- **HUD** (bottom, on the window layer; labels gold, numbers white): top row `MP` mana number + 10-cell mana bar (2 mana per cell) and `F` your food; bottom row `Q a/b` your queen HP / enemy queen HP, `A` your ants, `R` red ants, `P` popularity, and `FF` while fast forward is on.
- **Pause / help:** START opens a scrolling help list (controls, goal, mana, HUD key, elections, ants) under the HUD; UP/DOWN scrolls, hold to repeat, START resumes.
- **Music & sound:** in the game a 4-channel chiptune loop (arpeggio, lead, wave-channel bass and noise drums; A minor, 112 BPM, 8 bars) keeps playing while paused. Sound effects briefly borrow channels from it: blips for raise/lower/flood, food delivered, hatching, fights and queen hits. The music stops for the short win/lose jingles. The title screen has its own theme (see below).

## Disasters and perks (roguelike)
- **Disasters:** every so often (after the first ~12 s; never in sandbox or the tutorial) a random 3x3 patch of the map is hit, for both colonies alike: a **flash flood** sinks every tile one level, an **earthquake** shakes each tile up or down. Nests are safe. Ants on tiles that hit water drown. They come more often on HARD and less often on EASY.
- **Perks:** after every election that is not a coup, the HUD offers two random perks: `A:xxxxxxx B:xxxxxxx` (about 10 s to choose; press **A** or **B** to take that one, otherwise it is lost). Perks last for the rest of the run and are saved with the game:
  `FLOOD 4` flood costs 4 MP | `FASTEGG` your ants hatch from 2 food | `MANA UP` mana trickles twice as fast | `CALM` popularity sinks slower and disasters are rarer | `QUEENUP` +1 queen HP | `SLOWRED` red ants hatch from 1 more food.
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

## 2 players (versus)
On the title screen press **DOWN**. Pick with LEFT/RIGHT, START plays, B goes back.
- **SAME GB:** one Game Boy, take turns. Each turn is 30 seconds (the HUD shows **T** = seconds left). Then a card says whose turn it is: pass the Game Boy and press START.
- **LINK CABLE:** two Game Boys and a link cable. Both pick LINK CABLE, then ONE player presses START (that GB sends its level and map seed). Both games run in lockstep. The HUD shows **E** = enemy mana. Either player can pause; B twice in the pause menu quits on both. If the cable comes out you see LINK LOST.

P1 is black (gold cursor), P2 is red (red cursor). Both colonies play by the same rules, each player gets mana from their own ants' food. Kill the other queen to win. No elections, perks, ant eye, saves or records in versus.

## Build
`make GBDK_HOME=/path/to/gbdk` -> `build/empire-ants.gbc` (CGB-only ROM).

## Status
**Built through GitHub Actions using GBDK-2020, not yet validated on real hardware or linked emulators.** The CI workflow (`.github/workflows/build.yml`) checks the ROM and fixed-bank usage, runs a native navigation regression test and uploads the .gbc ROM as an artifact.

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
