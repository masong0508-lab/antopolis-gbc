#!/usr/bin/env python3
"""Check GBC pilot selection, concurrent controls, scent, and save continuity.

Original-format battery fixtures place workers on disconnected islands.
The real cartridge loads them through Continue and receives only ordinary
buttons. World memory is read, never patched. The fixture's exact worker
bytes locate its private array once, without hardcoding a WRAM address.
VRAM-bank selection is restored immediately after reading HUD glyphs.
"""
from __future__ import annotations

import argparse
import hashlib
import io
import json
from pathlib import Path
import re

from pyboy import PyBoy

from gbc_queen_safety_smoke import battery_fixture, checksum


GLYPHS = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ:!/"
CASES = ("switch-only", "switch-forward", "switch-backward", "switch-bite",
         "switch-scent", "stationary-scent", "entry-scent-held", "wet-entry", "wet-switch",
         "no-dry-worker", "food-save-continue")


def expanded_workers(ram):
    result = bytearray()
    for offset in range(466, 616, 3):
        x, y, flags = ram[offset:offset + 3]
        result.extend((x, y, (flags >> 1) & 1, flags & 1,
                       (flags >> 2) & 1, (flags >> 3) & 1))
    return bytes(result)


def fixture(name):
    ram = bytearray(battery_fixture(False, False).getvalue())
    ram[73:75] = bytes((10, 10))
    heights = [0] * 1024
    # The islands share no walkable path: an apparent short step between
    # them can only come from applying a new pilot to stale camera data.
    for y, first, last in ((10, 8, 12), (22, 20, 24)):
        for x in range(first, last + 1):
            heights[y * 32 + x] = 1
    heights[26 * 32 + 4] = heights[5 * 32 + 27] = 1
    workers = [(10, 10, 1), (22, 22, 1)]
    if name == "switch-bite":
        workers += [(11, 10, 3), (23, 22, 3)]
    if name in ("wet-entry", "no-dry-worker"):
        heights[10 * 32 + 10] = 0
    if name == "wet-switch":
        heights[22 * 32 + 22] = 0
        workers += [(20, 22, 1)]
    if name == "no-dry-worker":
        workers = workers[:1]
    ram[82:338] = bytes(
        sum(heights[i + j] << (j * 2) for j in range(4))
        for i in range(0, 1024, 4)
    )
    ram[338:466] = bytes(128)
    if name in ("switch-forward", "food-save-continue"):
        x, y = (23, 22) if name == "switch-forward" else (11, 10)
        ram[338 + y * 4 + x // 8] |= 1 << (x & 7)
    ram[466:616] = bytes(150)
    for i, worker in enumerate(workers):
        ram[466 + i * 3:469 + i * 3] = bytes(worker)
    ram[616] = checksum(ram[64:616])
    return bytes(ram), len(workers)


class Cartridge:
    def __init__(self, rom, name, out):
        self.ram, self.worker_count = fixture(name)
        self.out = out
        self.pyboy = PyBoy(str(rom), ram_file=io.BytesIO(self.ram),
                           window="null", cgb=True, sound_volume=0)
        self.pyboy.set_emulation_speed(0)
        symbols = rom.with_suffix(".map").read_text()
        self.address = {}
        for name in ("cx", "cy", "hgt", "food", "ph", "occ"):
            match = re.search(r"([0-9A-Fa-f]{8})\s+_" + name + r"\b", symbols)
            assert match, "Missing world symbol: " + name
            self.address[name] = int(match[1], 16)
            assert 0xC000 <= self.address[name] < 0xE000

    def tick(self, frames):
        self.pyboy.tick(frames, True, False)

    def press(self, key, frames=18, after=6):
        self.pyboy.button_press(key)
        self.tick(frames)
        self.pyboy.button_release(key)
        self.tick(after)

    def row(self, y):
        bank = self.pyboy.memory[0xFF4F]
        self.pyboy.memory[0xFF4F] = 0
        try:
            start = 0x9C00 + y * 32
            return "".join(GLYPHS[v - 136] if 136 <= v < 176 else "?"
                           for v in self.pyboy.memory[start:start + 20])
        finally:
            self.pyboy.memory[0xFF4F] = bank & 1

    def cursor(self):
        return [self.pyboy.memory[self.address[name]] for name in ("cx", "cy")]

    def tile(self, name, x, y):
        return self.pyboy.memory[self.address[name] + y * 32 + x]

    def workers(self):
        return [{"x": self.pyboy.memory[self.worker_address + 6 * i],
                 "y": self.pyboy.memory[self.worker_address + 6 * i + 1],
                 "team": self.pyboy.memory[self.worker_address + 6 * i + 2],
                 "alive": self.pyboy.memory[self.worker_address + 6 * i + 3],
                 "carry": self.pyboy.memory[self.worker_address + 6 * i + 4]}
                for i in range(self.worker_count)]

    def state(self):
        return {"cursor": self.cursor(), "workers": self.workers(),
                "header": self.row(0), "ahead": self.row(1),
                "feedback": self.row(14), "controls": self.row(17),
                "old_scent": self.tile("ph", 10, 10),
                "new_scent": self.tile("ph", 22, 22),
                "window_y": self.pyboy.memory[0xFF4A]}

    def snapshot(self, suffix):
        self.pyboy.screen.image.save(self.out / (suffix + ".png"))

    def load_save(self, boot=True, saved=None, hold_entry_keys=False):
        ram = self.ram if saved is None else saved
        if boot:
            self.tick(680)
            self.press("start", 3)
            self.tick(300)
        assert "UP:CONTINUE" in self.row(10), "Original-format save rejected"
        self.press("up", 3, 0)
        # The map window is installed after the entire save is loaded,
        # before its fade ends and before the first live simulation slice.
        for _ in range(1600):
            if self.pyboy.memory[0xFF4A] == 128 and self.cursor() == list(ram[73:75]):
                break
            self.tick(1)
        else:
            raise AssertionError("Saved world did not finish loading")
        world = bytes(self.pyboy.memory[0xC000:0xE000])
        pattern = expanded_workers(ram)
        offset = world.find(pattern)
        assert offset >= 0 and world.find(pattern, offset + 1) < 0, "Worker fixture is not unique"
        self.worker_address = 0xC000 + offset
        self.pyboy.button_press("select")
        self.pyboy.button_press("start")
        entered = False
        for _ in range(1200):
            self.tick(1)
            if self.row(0).startswith("ANT EYE"):
                entered = True
                break
            if "NO WORKER" in self.row(1):
                break
        else:
            raise AssertionError("Neither ANT EYE nor its no-worker feedback appeared")
        if not hold_entry_keys:
            self.pyboy.button_release("start")
            self.pyboy.button_release("select")
        self.tick(260 if entered else 2)
        return entered

    def chord(self, keys):
        for key in keys:
            self.pyboy.button_press(key)
        self.tick(18)
        held = self.state()
        if "select" in keys:
            self.tick(120)
            held = self.state()
            self.snapshot("held-scent")
        for key in keys:
            self.pyboy.button_release(key)
        self.tick(180)
        return held

    def battery(self):
        return bytes(self.pyboy.memory[0, 0xA000:0xA000 + 617])


def run_case(rom, name, out):
    out.mkdir(parents=True, exist_ok=True)
    cart = Cartridge(rom, name, out)
    record = {"case": name, "checks": [], "result": "RUNNING"}

    def check(label, passed):
        record["checks"].append({"name": label, "passed": bool(passed)})

    try:
        entered = cart.load_save(hold_entry_keys=name == "entry-scent-held")
        record["before"] = cart.state()
        cart.snapshot("before")
        if name == "no-dry-worker":
            check("submerged-only colony cannot enter ANT EYE", not entered)
            check("no-worker feedback remains on the map",
                  not entered and "NO WORKER" in cart.row(1) and cart.pyboy.memory[0xFF4A] == 128)
        elif name == "entry-scent-held":
            check("held entry START does not immediately exit", entered and cart.row(0).startswith("ANT EYE"))
            check("entry scent is placed without releasing SELECT", cart.tile("ph", 10, 10) == 255)
            check("entry scent is acknowledged before release or repress", cart.row(17) == "SCENT ON   START MAP")
            cart.snapshot("held-entry-scent")
            cart.pyboy.button_release("start")
            cart.tick(30)
            check("releasing only START keeps scent active", cart.row(17) == "SCENT ON   START MAP")
            cart.pyboy.button_release("select")
            cart.tick(60)
            check("releasing entry SELECT restores the controls", cart.row(17) == "SEL SCENT  START MAP")
        else:
            assert entered, "A dry worker must remain controllable"
            if name == "wet-entry":
                check("entry skips the nearest submerged worker", cart.cursor() == [22, 22])
            keys = {
                "switch-only": ("a",), "switch-forward": ("a", "up"),
                "switch-backward": ("a", "down"), "switch-bite": ("a", "b"),
                "switch-scent": ("a", "select"), "stationary-scent": ("select",),
                "wet-entry": ("up",), "wet-switch": ("a", "up"),
                "food-save-continue": ("up",),
            }[name]
            record["held"] = cart.chord(keys)
            record["after_action"] = cart.state()
            workers = cart.workers()
            if name == "switch-only":
                check("switching alone preserves both worker positions",
                      cart.cursor() == [22, 22] and workers[0]["x"] == 10 and workers[1]["x"] == 22)
                check("both separated ants remain in 3D occupancy",
                      cart.tile("occ", 10, 10) == 1 and cart.tile("occ", 22, 22) == 1)
            elif name == "switch-forward":
                check("A+UP moves from the new island", 22 < workers[1]["x"] <= 24 and workers[1]["y"] == 22)
                check("the old pilot stays in place", workers[0]["x"] == workers[0]["y"] == 10)
                check("the new pilot collects its own island's berry",
                      workers[1]["carry"] == 1 and cart.tile("food", 23, 22) == 0)
                check("only the old/new 3D cells change during walking",
                      cart.tile("occ", workers[1]["x"], 22) == 1 and
                      cart.tile("occ", 22, 22) == 0 and cart.tile("occ", 10, 10) == 1)
            elif name == "switch-backward":
                check("A+DOWN retreats on the new island", 20 <= workers[1]["x"] < 22 and workers[1]["y"] == 22)
                check("the old pilot stays in place", workers[0]["x"] == workers[0]["y"] == 10)
                check("retreat only refreshes changed 3D tiles",
                      cart.tile("occ", workers[1]["x"], 22) == 1 and
                      cart.tile("occ", 22, 22) == 0 and cart.tile("occ", 10, 10) == 1)
            elif name == "switch-bite":
                check("A+B attacks at the new pilot", workers[3]["alive"] == 0 and workers[2]["alive"] == 1)
                check("biting never moves either pilot", cart.cursor() == [22, 22] and workers[0]["x"] == 10 and workers[1]["x"] == 22)
                check("the successful bite is acknowledged", "ENEMY ANT DEFEATED" in cart.row(14))
                check("3D target clears dead ant without disturbing distant enemy",
                      cart.tile("occ", 23, 22) == 0 and cart.tile("occ", 11, 10) == 2)
            elif name in ("switch-scent", "stationary-scent"):
                x, y = (22, 22) if name == "switch-scent" else (10, 10)
                check("scent is placed at the selected worker", cart.tile("ph", x, y) == 255)
                if name == "switch-scent":
                    check("A+SELECT leaves the previous worker's tile unchanged",
                          cart.tile("ph", 10, 10) == record["before"]["old_scent"])
                check("holding scent visibly acknowledges the action",
                      record["held"]["controls"] == "SCENT ON   START MAP")
                check("releasing scent restores the controls", cart.row(17) == "SEL SCENT  START MAP")
            elif name in ("wet-entry", "wet-switch"):
                wet, dry = (0, 1) if name == "wet-entry" else (1, 2)
                wx, wy = (10, 10) if wet == 0 else (22, 22)
                check("the submerged worker is never rescued", workers[wet]["x"] == wx and workers[wet]["y"] == wy)
                check("movement uses the remaining dry worker", workers[dry]["y"] == 22 and cart.tile("hgt", workers[dry]["x"], 22) != 0)
                if name == "wet-switch":
                    check("switching skips the submerged middle slot", "ANT 3" in cart.row(0))
                cart.press("start", 18, 80)
                check("ordinary colony simulation still drowns that worker", cart.workers()[wet]["alive"] == 0)
            elif name == "food-save-continue":
                check("ordinary dry movement still collects food", workers[0]["carry"] == 1 and cart.tile("food", 11, 10) == 0)
                check("first-person food pickup leaves exact rendered occupancy",
                      cart.tile("occ", workers[0]["x"], 10) == 9 and cart.tile("occ", 10, 10) == 0)
                cart.press("start", 18, 6)
                cart.press("start", 18, 40)
                cart.press("a", 18, 40)
                check("pause save succeeds", "GAME SAVED!" in cart.row(16))
                saved = cart.battery()
                check("the original save checksum remains valid", checksum(saved[64:616]) == saved[616])
                check("the saved carrier retains its berry", bool(saved[468] & 4))
                record["saved_worker"] = list(saved[466:469])
                cart.press("b", 18, 40)
                cart.press("b", 18, 350)
                check("Continue remains offered", "UP:CONTINUE" in cart.row(10))
                check("the saved game can re-enter ANT EYE", cart.load_save(boot=False, saved=saved))
                check("Continue restores every worker byte",
                      bytes(cart.pyboy.memory[cart.worker_address:cart.worker_address + 300]) == expanded_workers(saved))
                check("Continue does not respawn collected food", cart.tile("food", 11, 10) == 0)
        record["final"] = cart.state()
        cart.snapshot("after")
        record["result"] = "PASS" if all(item["passed"] for item in record["checks"]) else "FAIL"
    except Exception as error:
        record["result"] = "FAIL"
        record["error"] = str(error)
        cart.snapshot("failure")
    finally:
        record["emulated_frames"] = cart.pyboy.frame_count
        cart.pyboy.stop(save=False)
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path)
    parser.add_argument("--out", type=Path, default=Path("build/pilot-safety"))
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    report = {"rom_sha256": hashlib.sha256(args.rom.read_bytes()).hexdigest(),
              "map_sha256": hashlib.sha256(args.rom.with_suffix(".map").read_bytes()).hexdigest(),
              "cases": [], "fixture": "Original-format battery saves; no live world or ROM patches"}
    for name in CASES:
        result = run_case(args.rom, name, args.out / name)
        report["cases"].append(result)
        print(name, result["result"], result.get("error", ""), flush=True)
    report["result"] = "PASS" if all(case["result"] == "PASS" for case in report["cases"]) else "FAIL"
    report["emulated_frames"] = sum(case["emulated_frames"] for case in report["cases"])
    (args.out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    return 0 if report["result"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
