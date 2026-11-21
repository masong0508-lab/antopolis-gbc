#!/usr/bin/env python3
"""Smoke-test the actual GBC cartridge in a headless Game Boy Color emulator.

Checks real boot, strategy play, entering 3D ANT EYE, camera turning and
returning to the same 2D world. Saves screenshot evidence to the build output.
Single-console validation; it does NOT claim link-cable interoperability.
"""
import argparse
import json
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

    try:
        # The intro studio splash has its own input gate. Earlier we used
        # 680 frames and mistook the splash for the title; the next START
        # only skipped the splash, leaving every screenshot one mode behind.
        # Explicitly advance through BOTH startup menus before ANT EYE.
        pyboy.tick(680, True, False)
        press("start")                   # skip DippInn studio splash
        pyboy.tick(300, True, False)
        snapshot("01_title")

        # A separate START begins normal single-player and spawns workers.
        press("start")
        pyboy.tick(350, True, False)
        snapshot("02_strategy")
        strategy_head = pyboy.memory[ANT_EYE_HEADER]
        if strategy_head != 136 + 23:  # 'M' from the MP HUD in row zero
            raise AssertionError(
                "Expected strategy HUD after title START, got window tile "
                + repr(strategy_head)
            )
        before = pyboy.memory[ANT_EYE_VIEW_ROW + 1]

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
        if len(eye.getcolors(maxcolors=160 * 144) or []) < 4:
            raise AssertionError("ANT EYE picture looks blank or uses too few colors")

        # Real camera turning should redraw the view, not freeze one half.
        press("right", 18)
        pyboy.tick(120, True, False)
        rotated = snapshot("04_turned_camera")
        if not ImageChops.difference(eye.convert("RGB"), rotated.convert("RGB")).getbbox():
            raise AssertionError("ANT EYE camera did not visibly update on turn")

        # Move the selected real worker, not merely the camera. Read the
        # 2-digit HUD coordinates on row 17; if a blocked facing is chosen,
        # rotate and try another reachable side. This catches regression
        # where the 3D camera animates but the ant never moves in-world.
        def position_glyphs():
            # CGB window tile-map data uses VRAM bank 0; the renderer
            # temporarily selects bank 1 for per-tile palette attributes.
            # Read the actual digits from bank 0, never attribute bytes,
            # otherwise a bank swap could falsely look like ant movement.
            base = WINDOW_MAP + 17 * 32
            previous_bank = pyboy.memory[0xFF4F]
            pyboy.memory[0xFF4F] = 0
            try:
                return tuple(pyboy.memory[base + col] for col in (1, 2, 5, 6))
            finally:
                pyboy.memory[0xFF4F] = previous_bank & 1

        before_pos = position_glyphs()
        moved = False
        for _ in range(16):
            pyboy.button_press("select")  # scent-mark the walked cell
            press("up", 4)
            pyboy.button_release("select")
            pyboy.tick(42, True, False)
            if not (pyboy.memory[0xFF40] & 0x80):
                raise AssertionError("ANT EYE LCD shut off while trying to walk")
            if position_glyphs() != before_pos:
                moved = True
                break
            press("right")
            pyboy.tick(38, True, False)
        if not moved:
            raise AssertionError("ANT EYE failed to move a real worker in 16 facings")
        report["movement"] = {
            "initial_coordinate_tiles": list(before_pos),
            "walked_coordinate_tiles": list(position_glyphs()),
            "real_worker_moved": True,
        }
        snapshot("05_walked_and_scent_marked")

        # Leave first person and make sure the regular 2D tile map returns.
        press("start")
        pyboy.tick(90, True, False)
        snapshot("06_back_to_strategy")
        returned = pyboy.memory[ANT_EYE_VIEW_ROW + 1]
        report["vram"]["return_row2_tile1"] = returned
        if returned == 12:
            raise AssertionError("Exited ANT EYE without restoring the map HUD")
        report["result"] = "PASS: GBC boot, strategy, 3D ANT EYE, turn, worker walking, scent, 2D return"
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
