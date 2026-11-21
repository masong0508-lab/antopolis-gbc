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

        # Leave first person and make sure the regular 2D tile map returns.
        press("start")
        pyboy.tick(90, True, False)
        snapshot("05_back_to_strategy")
        returned = pyboy.memory[ANT_EYE_VIEW_ROW + 1]
        report["vram"]["return_row2_tile1"] = returned
        if returned == 12:
            raise AssertionError("Exited ANT EYE without restoring the map HUD")
        report["result"] = "PASS: GBC boot, strategy map, real 3D ANT EYE, turn, 2D return"
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
