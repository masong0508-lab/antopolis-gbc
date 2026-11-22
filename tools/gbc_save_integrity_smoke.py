#!/usr/bin/env python3
"""Reject checksum-valid but out-of-world GBC battery saves before Continue.

The cartridge is never patched or given runtime RAM writes. Every
adversarial save uses the original MBC5 SRAM layout and a *correct*
rotate-XOR checksum, so the test catches missing semantic validation.
"""
from __future__ import annotations

import argparse
import hashlib
import io
import json
from pathlib import Path

from pyboy import PyBoy
from gbc_queen_safety_smoke import battery_fixture, checksum

GLYPHS = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ:!/"
CASES = ("valid-original", "invalid-cursor-x", "invalid-cursor-y",
         "invalid-difficulty", "invalid-live-x", "invalid-live-y",
         "stale-inactive-ok")
SAVE_START = 64
SAVE_END = 616


def fixture(which: str):
    ram = bytearray(battery_fixture(False, False).getvalue())
    if which == "invalid-cursor-x":
        ram[SAVE_START + 9] = 255
    elif which == "invalid-cursor-y":
        ram[SAVE_START + 10] = 255
    elif which == "invalid-difficulty":
        ram[SAVE_START + 1] = 3
    elif which == "invalid-live-x":
        ram[466] = 255
    elif which == "invalid-live-y":
        ram[467] = 255
    elif which == "stale-inactive-ok":
        ram[469] = ram[470] = 255  # slot 1 is dead; these values are ignored
        assert ram[471] == 0
    else:
        assert which == "valid-original"
    ram[SAVE_END] = checksum(ram[SAVE_START:SAVE_END])
    assert ram[SAVE_END] == checksum(ram[SAVE_START:SAVE_END])
    return io.BytesIO(ram)


def hud_row(pb, y):
    bank = pb.memory[0xFF4F]
    pb.memory[0xFF4F] = 0
    try:
        start = 0x9C00 + y * 32
        return "".join(GLYPHS[t - 136] if 136 <= t < 176 else "?"
                       for t in pb.memory[start:start + 20])
    finally:
        pb.memory[0xFF4F] = bank & 1


def run_case(rom: Path, name: str, out: Path):
    pb = PyBoy(str(rom), ram_file=fixture(name), window="null",
               cgb=True, sound_volume=0)
    pb.set_emulation_speed(0)
    result = {"case": name, "result": "RUNNING"}
    try:
        pb.tick(680, True, False)
        pb.button_press("start")
        pb.tick(3, True, False)
        pb.button_release("start")
        pb.tick(310, True, False)
        title = hud_row(pb, 10)
        expected_valid = name in ("valid-original", "stale-inactive-ok")
        actual_valid = "UP:CONTINUE" in title
        result["title_save_row"] = title
        result["expected_continue"] = expected_valid
        result["actual_continue"] = actual_valid
        pb.screen.image.save(out / (name + ".png"))
        if actual_valid != expected_valid:
            raise AssertionError("Invalid save offered Continue, or valid save rejected")
        if not expected_valid:
            pb.button_press("up")
            pb.tick(18, True, False)
            pb.button_release("up")
            pb.tick(32, True, False)
            if "UP:CONTINUE" in hud_row(pb, 10):
                raise AssertionError("Rejected corrupted save resurfaced after UP")
        result["result"] = "PASS"
    except Exception as exc:
        result["result"] = "FAIL"
        result["error"] = str(exc)
    finally:
        result["frames"] = pb.frame_count
        pb.stop(save=False)
    return result


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("rom", type=Path)
    ap.add_argument("--out", type=Path, default=Path("build/save-integrity"))
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    report = {"rom_sha256": hashlib.sha256(args.rom.read_bytes()).hexdigest(),
              "cases": [], "notes": "Correct-checksum original saves; no live patching"}
    for name in CASES:
        result = run_case(args.rom, name, args.out)
        report["cases"].append(result)
        print(name, result["result"], result.get("error", ""), flush=True)
    report["result"] = ("PASS" if all(x["result"] == "PASS"
                                    for x in report["cases"]) else "FAIL")
    report["emulated_frames"] = sum(x["frames"] for x in report["cases"])
    (args.out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    return 0 if report["result"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
