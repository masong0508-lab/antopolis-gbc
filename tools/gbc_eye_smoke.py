#!/usr/bin/env python3
"""Smoke-test the actual GBC cartridge in a headless Game Boy Color emulator.

Checks real boot, strategy play, entering 3D ANT EYE, camera turning and
returning to the same 2D world. Saves screenshot evidence to the build output.
Single-console validation; it does NOT claim link-cable interoperability.
"""
import argparse
import json
import re
from pathlib import Path

from pyboy import PyBoy
from PIL import ImageChops


WINDOW_MAP = 0x9C00
ANT_EYE_VIEW_ROW = WINDOW_MAP + 2 * 32
ANT_EYE_HEADER = WINDOW_MAP
ANT_EYE_TITLE_A = 136 + 11   # FT + gi('A') from src/main.c


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path)
    parser.add_argument("--out", type=Path, default=Path("build/eye-smoke"))
    parser.add_argument("--diagnostic-only", action="store_true",
                        help="assert LCD survives ANT EYE entry in a no-ray debug ROM")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    report = {"rom": str(args.rom), "snapshots": {}, "result": "RUNNING"}
    pyboy = PyBoy(str(args.rom), window="null", cgb=True, sound_volume=0)
    pyboy.set_emulation_speed(0)
    last = None

    # Check actual mutable game state, not a UI digit that may rightly be
    # replaced by gameplay goals, food pickups, or battle notifications.
    map_path = args.rom.with_suffix(".map")
    symbols = map_path.read_text()
    def world_address(symbol):
        match = re.search(r"([0-9A-Fa-f]{8})\s+_" + symbol + r"\b", symbols)
        if not match:
            raise AssertionError("Expected exported game RAM symbol: " + symbol)
        address = int(match.group(1), 16)
        if not 0xC000 <= address < 0xE000:
            raise AssertionError("Game cursor not in WRAM: " + symbol)
        return address
    cursor_x, cursor_y = world_address("cx"), world_address("cy")
    def world_position():
        return pyboy.memory[cursor_x], pyboy.memory[cursor_y]

    def snapshot(name):
        nonlocal last
        image = pyboy.screen.image.copy()
        path = args.out / (name + ".png")
        image.save(path)
        frame_diff = None if last is None else bool(ImageChops.difference(last.convert("RGB"), image.convert("RGB")).getbbox())
        report["snapshots"][name] = {
            "path": path.name,
            "size": list(image.size),
            "changed_from_previous": frame_diff,
            "colors": len(image.getcolors(maxcolors=160 * 144) or []),
        }
        last = image
        return image

    def press(name, frames=3):
        pyboy.button_press(name)
        pyboy.tick(frames, True, False)
        pyboy.button_release(name)
        pyboy.tick(6, True, False)

    def tile0(x, y):
        # Window glyph IDs are in CGB VRAM bank 0, not attribute bank 1.
        bank = pyboy.memory[0xFF4F]
        pyboy.memory[0xFF4F] = 0
        try:
            return pyboy.memory[WINDOW_MAP + y * 32 + x]
        finally:
            pyboy.memory[0xFF4F] = bank & 1

    def glyph(letter):
        # The cartridge's compact font does not use ASCII tile indices:
        # space, digits, letters and punctuation have separate slots.
        if letter == " ":
            return 136
        if "0" <= letter <= "9":
            return 137 + ord(letter) - ord("0")
        if "A" <= letter <= "Z":
            return 147 + ord(letter) - ord("A")
        return {":": 173, "!": 174, "/": 175}[letter]

    def text_tile(character):
        # The generated GBC font maps punctuation outside the A-Z glyph run.
        if character == " ":
            return 136
        if character == ":":
            return 136 + 37
        if character == "!":
            return 136 + 38
        if character == "/":
            return 136 + 39
        if "0" <= character <= "9":
            return 136 + 1 + ord(character) - ord("0")
        return glyph(character)

    try:
        # The intro studio splash has its own input gate. Earlier we used
        # 680 frames and mistook the splash for the title; the next START
        # only skipped the splash, leaving every screenshot one mode behind.
        # Explicitly advance through BOTH startup menus before ANT EYE.
        pyboy.tick(680, True, False)
        press("start")                   # skip DippInn studio splash
        pyboy.tick(300, True, False)
        snapshot("01_title")
        # Visual identity regression: the new raised 5x7 A has a lit top
        # row, whereas the former 3x5 typeface always left it empty.
        bank = pyboy.memory[0xFF4F]
        pyboy.memory[0xFF4F] = 0
        try:
            capital_a_first_row = pyboy.memory[0x8000 + glyph("A") * 16]
        finally:
            pyboy.memory[0xFF4F] = bank & 1
        if capital_a_first_row != 0x38:
            raise AssertionError(
                "5x7 interface font not installed: " + repr(capital_a_first_row)
            )
        if tile0(0, 8) != 203:
            raise AssertionError("Title bronze separator missing")
        if tile0(0, 11) != glyph("A") or tile0(1, 12) != glyph("S"):
            raise AssertionError("Title tutorial/sandbox shortcuts are missing")

        # A separate START begins normal single-player and spawns workers.
        press("start")
        pyboy.tick(350, True, False)
        snapshot("02_strategy")
        # Check the directional seam against the ACTUAL live world heights.
        # The previous >=100 random-accents count does not apply when tiles
        # are reserved for true shorelines/cliffs. This catches inverted
        # edges, stale seams and ROM-bank render errors on a real cartridge.
        height_base = world_address("hgt")
        def height_at(x, y):
            return pyboy.memory[height_base + 32 * y + x]
        def expected_edge(x, y):
            h = height_at(x, y)
            if h not in (0, 3):
                return 0
            around = (
                height_at(x, y - 1) if y else h,
                height_at(x, y + 1) if y < 31 else h,
                height_at(x - 1, y) if x else h,
                height_at(x + 1, y) if x < 31 else h,
            )
            for direction, adjacent in enumerate(around):
                if (h == 0 and adjacent > 0) or (h == 3 and adjacent < 3):
                    return 1 + direction + (4 if h == 3 else 0)
            return 0
        bank = pyboy.memory[0xFF4F]
        pyboy.memory[0xFF4F] = 0
        try:
            detail_tiles = 0
            edge_types = set()
            for y in range(32):
                for x in range(32):
                    tile = pyboy.memory[0x9800 + y * 32 + x]
                    if 204 <= tile <= 211:
                        want = expected_edge(x, y)
                        if tile != 203 + want:
                            raise AssertionError(
                                f"World transition at ({x},{y}): tile {tile} "
                                f"but terrain requires {203 + want}"
                            )
                        detail_tiles += 1
                        edge_types.add("shore" if tile <= 207 else "cliff")
        finally:
            pyboy.memory[0xFF4F] = bank & 1
        if detail_tiles < 12:
            raise AssertionError(
                "Actual shore/cliff geometry has no visible transitions: "
                + repr(detail_tiles)
            )
        report["shoreline_and_cliff_tiles"] = {
            "count": detail_tiles, "types": sorted(edge_types),
            "aligned_to_live_heights": True
        }
        strategy_head = pyboy.memory[ANT_EYE_HEADER]
        if strategy_head != 136 + 23:  # 'M' from the MANA HUD in row zero
            raise AssertionError(
                "Expected strategy HUD after title START, got window tile "
                + repr(strategy_head)
            )
        # Status band is understandable without opening the manual.
        if tile0(1, 0) != glyph("A") or tile0(14, 0) != glyph("F"):
            raise AssertionError("Strategy HUD lost MANA/FOOD resource labels")
        before = pyboy.memory[ANT_EYE_VIEW_ROW + 1]
        terrain_tile = tile0(5, 1)
        if terrain_tile not in {glyph(c) for c in "WSGH"}:
            raise AssertionError("Strategy HUD does not show the cursor terrain")
        report["ui"] = {"title_shortcuts": True,
                        "cursor_terrain_tile": terrain_tile,
                        "raised_font_first_row": capital_a_first_row,
                        "detailed_world_cells": detail_tiles}

        # Check the *real hardware OAM*, not merely source sprite assets:
        # the same two ant walking frames used on the title must animate
        # workers during actual overhead gameplay. Read OAM slots 4..39;
        # the first four are reserved for queens/player cursors, which must
        # remain visible even when 10 workers share a hardware scanline.
        def visible_worker_frames():
            workers = {}
            for sprite in range(4, 40):
                oam = 0xFE00 + 4 * sprite
                sy, sx, frame = (pyboy.memory[oam + j] for j in (0, 1, 2))
                if 16 <= sy < 144 and 8 <= sx < 168:
                    workers[sprite] = frame
            return workers

        # Both royal silhouettes occupy the highest priority OAM entries,
        # followed by the player cursor, and must retain their correct
        # sprite tile IDs after the title parade recycled those slots.
        if any(pyboy.memory[0xFE00 + 4 * i + 2] != expected
               for i, expected in ((0, 2), (1, 2), (2, 1))):
            raise AssertionError("Queen/player OAM priority or silhouettes lost")
        # Observe several completed game redraws even on a slow 160x144
        # cartridge. Four 8-frame snapshots are too short when the host
        # emulator advances VBlank faster than the banked simulation.
        walker_samples = [visible_worker_frames()]
        clock_base = world_address("sys_time")
        def vblank_clock():
            return (pyboy.memory[clock_base]
                    | (pyboy.memory[clock_base + 1] << 8))
        clocks = [vblank_clock()]
        for _ in range(16):
            pyboy.tick(8, True, False)
            walker_samples.append(visible_worker_frames())
            clocks.append(vblank_clock())

        # Only count a change *within* the authentic 3/4 walking or 5/6
        # carrying pair; a worker dropping food (5 -> 3) is not animation.
        animated = []
        for slot in range(4, 40):
            frames = [sample.get(slot) for sample in walker_samples]
            if any(
                a is not None and b is not None and a != b
                and ({a, b} == {3, 4} or {a, b} == {5, 6})
                for a, b in zip(frames, frames[1:])
            ):
                animated.append(slot)
        report["ui"]["walker_frames_sampled"] = [
            {str(i): frame for i, frame in sorted(sample.items())}
            for sample in walker_samples
        ]
        report["ui"]["vblank_clocks"] = clocks
        if not any(walker_samples):
            raise AssertionError("No on-screen colony worker OAM sprites")
        if not animated:
            raise AssertionError(
                "Overhead GBC workers failed to alternate walking (3/4) "
                "or carrying (5/6) OAM tiles across 128 emulated frames; "
                "clocks=" + repr(clocks)
                + ", frames=" + repr(report["ui"]["walker_frames_sampled"])
            )
        report["ui"]["walking_ants_animated"] = True

        # Pause/help is a navigable full-screen guide, not just a label.
        press("start", 18)
        pyboy.tick(35, True, False)
        if pyboy.memory[0xFF4A] != 0 or tile0(0, 2) != glyph("P"):
            raise AssertionError("Pause guide failed to open")
        if tile0(3, 0) != glyph("C") or tile0(0, 0) != 136:
            raise AssertionError("Pause guide did not clear old HUD and draw command header")
        if tile0(0, 1) != 203:
            raise AssertionError("Command screen divider is missing")
        # Verify the shipped mission-first help text, not an obsolete
        # START HERE heading from the prior cartridge revision.
        top_help_tile = tile0(0, 3)
        if top_help_tile != glyph("Y"):  # #YOUR MISSION in bank 2
            raise AssertionError("Pause guide did not open at YOUR MISSION")
        for col, letter in enumerate("SAVE YOUR QUEEN"):
            if tile0(col, 4) != glyph(letter):
                raise AssertionError(
                    "Pause guide lost readable queen objective at column "
                    + str(col)
                )
        for col, letter in enumerate("DEFEAT THE RED QUEEN"):
            if tile0(col, 5) != glyph(letter):
                raise AssertionError(
                    "Pause guide lost its enemy queen objective at column "
                    + str(col)
                )
        snapshot("02_pause_guide")
        press("down", 18)
        pyboy.tick(20, True, False)
        if tile0(0, 3) == top_help_tile:
            raise AssertionError("Pause guide did not scroll with DOWN")
        report["ui"]["help_scrolled"] = True
        snapshot("02_scrolled_guide")
        press("start", 18)
        pyboy.tick(50, True, False)
        if pyboy.memory[0xFF4A] != 128:
            raise AssertionError("Pause guide did not restore the strategy HUD")
        if tile0(0, 0) != glyph("M") or tile0(0, 1) != glyph("Q"):
            raise AssertionError("Pause resume did not repaint gameplay HUD")
        report["ui"]["help_resumed"] = True

        # ANT EYE entry is a SELECT+START combo. Record PPU hardware state
        # throughout transition: a solid-green frame can mean LCD was
        # disabled, not merely a wrongly colored 3D tile.
        def ppu_state(stage):
            report.setdefault("ppu_checkpoints", {})[stage] = {
                "lcdc": pyboy.memory[0xFF40],
                "vbk": pyboy.memory[0xFF4F],
                "wx": pyboy.memory[0xFF4B],
                "wy": pyboy.memory[0xFF4A],
                "ly": pyboy.memory[0xFF44],
                "header": pyboy.memory[ANT_EYE_HEADER],
                "world_row": pyboy.memory[ANT_EYE_VIEW_ROW],
                "pixel": list(pyboy.screen.image.getpixel((0, 0))),
            }

        ppu_state("strategy")
        pyboy.button_press("select")
        pyboy.tick(3, True, False)  # physical SELECT first, then START
        pyboy.button_press("start")
        pyboy.tick(5, True, False)
        ppu_state("combo_held")
        pyboy.button_release("start")
        pyboy.button_release("select")
        pyboy.tick(8, True, False)
        ppu_state("after_release")
        pyboy.tick(22, True, False)
        ppu_state("30_frames")
        pyboy.tick(150, True, False)
        ppu_state("180_frames")
        eye = snapshot("03_ant_eye")
        # Audit ACTUAL 160x144 native LCD pixels, not a renderer mockup.
        # Grass, rock and foam must remain visible over the ray floor,
        # the three-band sky must not collapse, and ANT EYE HUD survives.
        sky_colors = len(eye.convert("RGB").crop((0, 20, 160, 60)).getcolors(
            maxcolors=160*40) or [])
        floor_colors = len(eye.convert("RGB").crop((0, 74, 160, 108)).getcolors(
            maxcolors=160*34) or [])
        report["native_eye_visual"] = {
            "resolution": [160, 144], "sky_colors": sky_colors,
            "ground_colors": floor_colors,
            "landmark": "ANT EYE native scene",
        }
        if sky_colors < 3 or floor_colors < 2:
            raise AssertionError(
                "Native ANT EYE sky/ground lost material depth: "
                + repr(report["native_eye_visual"])
            )
        if args.diagnostic_only:
            lcdc = report["ppu_checkpoints"]["180_frames"]["lcdc"]
            if not (lcdc & 0x80):
                raise AssertionError(
                    "LCD disabled even with raymarch skipped: "
                    + repr(report["ppu_checkpoints"])
                )
            report["result"] = "PASS: no-ray diagnostic kept ANT EYE LCD enabled"
            return
        top = pyboy.memory[ANT_EYE_HEADER]
        first = pyboy.memory[ANT_EYE_VIEW_ROW]
        second = pyboy.memory[ANT_EYE_VIEW_ROW + 1]
        report["vram"] = {
            "strategy_row2_tile1": before,
            "eye_header_tile": top,
            "eye_row2_tile0": first,
            "eye_row2_tile1": second,
        }
        if (top, first, second) != (ANT_EYE_TITLE_A, 0, 12):
            raise AssertionError(
                "SELECT+START never loaded the ANT EYE header and 3D tiles: "
                + repr(report["vram"])
            )
        # The upper sky has its own CGB palette (7), and the 1px terrain
        # grain leaves non-solid bitplane bytes in bank 1's ray tiles.
        bank = pyboy.memory[0xFF4F]
        pyboy.memory[0xFF4F] = 1
        try:
            upper_sky_palette = pyboy.memory[WINDOW_MAP + 2 * 32] & 7
            grain_bytes = sum(
                0 < pyboy.memory[0x8000 + i] < 255 for i in range(240 * 16)
            )
        finally:
            pyboy.memory[0xFF4F] = bank & 1
        report["render_detail"] = {"upper_sky_palette": upper_sky_palette,
                                   "textured_vram_bytes": grain_bytes}
        if upper_sky_palette != 7 or grain_bytes < 20:
            raise AssertionError("ANT EYE lost atmospheric sky / terrain detail")

        # The horizon should brighten through actual blue tones, not
        # remain the previous near-black monolithic upper rectangle.
        sky_samples = [eye.convert("RGB").getpixel((24, y)) for y in (20, 28, 36)]
        report["render_detail"]["sky_gradient_rgb"] = sky_samples
        if len(set(sky_samples)) < 3:
            raise AssertionError("ANT EYE upper sky lacks its three-layer atmosphere")
        if any(blue < red for red, green, blue in sky_samples):
            raise AssertionError("ANT EYE sky contains a mismatched red/yellow band")
        if len(eye.getcolors(maxcolors=160 * 144) or []) < 4:
            raise AssertionError("ANT EYE picture looks blank or uses too few colors")
        # Read VRAM bank 0: the foreground palette attribute plane is bank 1.
        bank_saved = pyboy.memory[0xFF4F]
        pyboy.memory[0xFF4F] = 0
        try:
            look_tile = pyboy.memory[WINDOW_MAP + 1 * 32 + 6]
        finally:
            pyboy.memory[0xFF4F] = bank_saved & 1
        # Actual window tile-map, not source-code assumptions: readable
        # word-based target, selected worker and four non-overlapping rows.
        target_tile = tile0(7, 1)
        if target_tile not in {glyph(c) for c in "MWSCOFEHR"}:
            raise AssertionError("AHEAD target is not a readable world label")
        if (tile0(0, 0) != glyph("A") or tile0(8, 0) != glyph("A")
                or tile0(14, 0) != glyph("D")):
            raise AssertionError("ANT EYE selected ant / DIR header is absent")
        if (tile0(0, 15), tile0(0, 16), tile0(0, 17)) != (
            glyph("U"), glyph("A"), glyph("S")
        ):
            raise AssertionError("ANT EYE commands overlap the wrong rows")
        # The playable HUD now prioritizes a clear goal over raw X/Y debug
        # coordinates. Verify the actual VRAM row, not just formatting code.
        objective_prefixes = (
            "FOOD:", "HOME:", "DETOUR:", "NO FOOD NEARBY",
            "FOOD PICKED UP", "FOOD DELIVERED"
        )
        if not any(all(tile0(i, 14) == text_tile(ch)
                       for i, ch in enumerate(prefix))
                   for prefix in objective_prefixes):
            raise AssertionError("ANT EYE status lost its readable objective")
        report["hud_target_tile"] = target_tile
        report["ui"]["eye_commands_grouped"] = True

        # Real camera turning should redraw the view, not freeze one half.
        press("right", 18)
        pyboy.tick(120, True, False)
        rotated = snapshot("04_turned_camera")
        if not ImageChops.difference(eye.convert("RGB"), rotated.convert("RGB")).getbbox():
            raise AssertionError("ANT EYE camera did not visibly update on turn")

        # Track ray-tile VRAM rather than the obsolete coordinate HUD.
        # Native movement/collision tests verify in-world coordinates; the
        # emulator checks that walking visibly changes the 3D world, not just
        # the camera heading. Sample the entire low-cost 240-tile viewport.
        def scene_signature():
            bank = pyboy.memory[0xFF4F]
            pyboy.memory[0xFF4F] = 1
            try:
                return bytes(pyboy.memory[0x8000 + i]
                             for i in range(0, 240 * 16, 11))
            finally:
                pyboy.memory[0xFF4F] = bank & 1

        moved = False
        for _ in range(16):
            # Do not hold scent while testing movement: the new trail would
            # change terrain pixels even if the ant walked into a wall.
            pyboy.tick(48, True, False)
            before_scene = scene_signature()
            press("up", 18)
            pyboy.tick(64, True, False)
            if not (pyboy.memory[0xFF40] & 0x80):
                raise AssertionError("ANT EYE LCD shut off while trying to walk")
            if scene_signature() != before_scene:
                moved = True
                break
            press("right", 16)
            pyboy.tick(48, True, False)
        if not moved:
            raise AssertionError("ANT EYE failed to update its 3D scene after walking")
        report["movement"] = {
            "walk_changed_world_view": True,
            "collision_coordinates_checked_by_native_suite": True,
        }
        # Keep the scent mechanic exercised independently of movement.
        pyboy.button_press("select")
        press("up", 18)
        pyboy.button_release("select")
        pyboy.tick(64, True, False)
        snapshot("05_walked_and_scent_marked")

        # Biting must surface an actionable result and keep the LCD active.
        press("b", 18)
        feedback_letter = tile0(0, 14)
        if feedback_letter not in {glyph("N"), glyph("E"), glyph("R")}:
            raise AssertionError(
                "ANT EYE bite gave no legible outcome feedback: "
                + repr(feedback_letter)
            )
        report["combat_feedback_tile"] = feedback_letter
        pyboy.tick(84, True, False)
        if tile0(0, 14) not in {glyph("N"), glyph("E"), glyph("R")}:
            raise AssertionError("ANT EYE feedback disappeared before another action")
        # Turning dismisses the action result and restores a useful goal.
        press("right", 18)
        pyboy.tick(85, True, False)
        if not any(all(tile0(i, 14) == text_tile(ch)
                       for i, ch in enumerate(prefix))
                   for prefix in objective_prefixes):
            raise AssertionError("ANT EYE turn failed to restore readable directions")
        if not (pyboy.memory[0xFF40] & 0x80):
            raise AssertionError("ANT EYE bite shut off the GBC display")
        report["combat"] = {"action_feedback_and_navigation_restored": True}
        snapshot("06_bite")

        # Leave first person and make sure the regular 2D tile map returns.
        # A ray sweep may span more than three emulated frames; hold START
        # long enough for the game to sample it rather than sending an
        # impossibly short emulator-only tap during VRAM uploads.
        press("start", 18)
        pyboy.tick(120, True, False)
        snapshot("07_back_to_strategy")
        vbk = pyboy.memory[0xFF4F]
        pyboy.memory[0xFF4F] = 0
        try:
            returned = pyboy.memory[ANT_EYE_VIEW_ROW + 1]
        finally:
            pyboy.memory[0xFF4F] = vbk & 1
        report["vram"]["return_row2_tile1"] = returned
        report["vram"]["return_window_y"] = pyboy.memory[0xFF4A]
        if returned == 12 or pyboy.memory[0xFF4A] != 128:
            raise AssertionError(
                "ANT EYE did not restore the 2D HUD and window position: "
                + repr(report["vram"])
            )
        # Repeated switches catch stale window maps, VRAM palette, or bank
        # state after a completed walking/combat session.
        pyboy.button_press("select")
        pyboy.tick(3, True, False)
        pyboy.button_press("start")
        pyboy.tick(18, True, False)
        pyboy.button_release("start")
        pyboy.button_release("select")
        pyboy.tick(210, True, False)
        if not (pyboy.memory[0xFF40] & 0x80):
            raise AssertionError("Second ANT EYE entry disabled LCD")
        bank_saved = pyboy.memory[0xFF4F]
        pyboy.memory[0xFF4F] = 0
        try:
            second_header = pyboy.memory[ANT_EYE_HEADER]
        finally:
            pyboy.memory[0xFF4F] = bank_saved & 1
        if second_header != ANT_EYE_TITLE_A:
            raise AssertionError("Second ANT EYE entry has an invalid header")
        snapshot("08_reentered_ant_eye")
        press("start", 18)
        pyboy.tick(110, True, False)
        if not (pyboy.memory[0xFF40] & 0x80) or pyboy.memory[0xFF4A] != 128:
            raise AssertionError("Second ANT EYE exit did not restore the 2D map")
        report["repeat_transitions"] = "PASS: second 2D/3D entry and exit"
        snapshot("09_second_return_to_strategy")
        report["result"] = "PASS: GBC 3D ANT EYE, turn, walk, scent, bite, repeat 2D/3D transitions"
        print(report["result"])
    except Exception as error:
        report["result"] = "FAIL: " + repr(error)
        raise
    finally:
        pyboy.stop()
        (args.out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
