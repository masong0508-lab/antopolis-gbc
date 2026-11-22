#!/usr/bin/env python3
"""Check actual GBC palette colors through title and colony fades.

Read the hardware's RGB555 palette ports after each emulated frame. The
index registers are restored before emulation resumes; no live game state
or cartridge instructions are changed. An optional baseline ROM produces
a direct transition-frame comparison using the same ordinary input.
"""
from __future__ import annotations

import argparse
import hashlib
import io
import json
from pathlib import Path
import re

from pyboy import PyBoy


def palette_reference():
    source = (Path(__file__).resolve().parents[1] / "src/palette_fade.c").read_text()
    palettes = {}
    for name, count in (("bpal", 32), ("tpal", 32), ("spal", 28)):
        block = re.search(r"\b" + name + r"\[\d+\]\s*=\s*\{(.*?)\};", source, re.S)
        assert block, "Palette table missing: " + name
        colors = []
        for red, green, blue in re.findall(r"RGB\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)", block[1]):
            r, g, b = int(red), int(green), int(blue)
            assert max(r, g, b) <= 31
            colors.append(r | (g << 5) | (b << 10))
        assert len(colors) == count
        palettes[name] = colors

    def dim(color, level):
        r = ((color & 31) * level) >> 3
        g = (((color >> 5) & 31) * level) >> 3
        b = (((color >> 10) & 31) * level) >> 3
        return r | (g << 5) | (b << 10)

    return {
        (scene, level): ([dim(c, level) for c in palettes[name]],
                         [dim(c, level) for c in palettes["spal"]])
        for scene, name in (("title", "tpal"), ("game", "bpal"))
        for level in range(9)
    }


def check_rom(rom, out, expected):
    record = {"rom_sha256": hashlib.sha256(rom.read_bytes()).hexdigest(),
              "events": [], "result": "RUNNING"}
    pyboy = PyBoy(str(rom), ram_file=io.BytesIO(bytes(8192)), window="null",
                  cgb=True, sound_volume=0)
    pyboy.set_emulation_speed(0)

    def read_palette(index_register, data_register, count):
        saved_index = pyboy.memory[index_register]
        colors = []
        try:
            for i in range(count):
                pyboy.memory[index_register] = i * 2
                low = pyboy.memory[data_register]
                pyboy.memory[index_register] = i * 2 + 1
                high = pyboy.memory[data_register]
                colors.append(low | (high << 8))
        finally:
            pyboy.memory[index_register] = saved_index
        return colors

    def current_palette():
        background = read_palette(0xFF68, 0xFF69, 32)
        objects = read_palette(0xFF6A, 0xFF6B, 28)
        matches = [f"{scene}:{level}" for (scene, level), (bg, obj) in expected.items()
                   if background == bg and objects == obj]
        return "+".join(matches) if matches else "partial"

    try:
        pyboy.tick(680, True, False)
        pyboy.button_press("start")
        pyboy.tick(3, True, False)
        pyboy.button_release("start")
        pyboy.tick(300, True, False)
        assert current_palette() == "title:8", "Title palette is not at full brightness"
        pyboy.screen.image.save(out / "title.png")
        record["events"].append({"relative_frame": 0, "palette": "title:8"})

        # Normal START, followed only by frame advances. No timing, CPU,
        # world, palette-data or source-ROM patch is used by this test.
        pyboy.button_press("start")
        for frame in range(1, 901):
            pyboy.tick(1, True, False)
            if frame == 3:
                pyboy.button_release("start")
            current = current_palette()
            if current != record["events"][-1]["palette"]:
                record["events"].append({"relative_frame": frame, "palette": current})

        assert current_palette() == "game:8", "Colony palette did not reach full brightness"
        pyboy.screen.image.save(out / "colony.png")
        events = record["events"]
        for scene in ("title", "game"):
            levels = {int(tag.split(":")[1]) for event in events
                      for tag in event["palette"].split("+") if tag.startswith(scene + ":")}
            assert levels == set(range(9)), f"{scene}: missing RGB555 brightness levels {set(range(9)) - levels}"
            record[scene + "_brightness_levels"] = sorted(levels)
        black = [e["relative_frame"] for e in events if e["palette"] == "title:0+game:0"]
        full = [e["relative_frame"] for e in events if e["palette"] == "game:8"]
        assert len(black) >= 2 and full[-1] > black[-1]
        record["title_black_frame"] = black[0]
        record["colony_full_frame"] = full[-1]
        record["colony_fade_frames"] = full[-1] - black[-1]
        record["result"] = "PASS"
    except Exception as error:
        record["result"] = "FAIL"
        record["error"] = str(error)
        pyboy.screen.image.save(out / "failure.png")
    finally:
        record["emulated_frames"] = pyboy.frame_count
        pyboy.stop(save=False)
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path)
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--out", type=Path, default=Path("build/palette-transitions"))
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    expected = palette_reference()
    report = {"current": check_rom(args.rom, args.out, expected)}
    if args.baseline:
        baseline_out = args.out / "baseline"
        baseline_out.mkdir(exist_ok=True)
        report["baseline"] = check_rom(args.baseline, baseline_out, expected)
    report["result"] = "PASS" if all(r["result"] == "PASS" for r in report.values()) else "FAIL"
    (args.out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    for name in ("current", "baseline"):
        if name in report:
            item = report[name]
            print(name, {k: v for k, v in item.items() if k not in ("events", "rom_sha256")})
    return 0 if report["result"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
