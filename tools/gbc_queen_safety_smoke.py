#!/usr/bin/env python3
"""Exercise practice-mode queen survival in the real GBC cartridge.

Valid battery saves place a worker beside the enemy queen or a full
attacking colony on an isolated enemy nest. No production RAM writes or
debug ROM are used. Normal-mode controls must still reach victory.
"""
import argparse
import hashlib
import io
import json
import re
from pathlib import Path

from pyboy import PyBoy


GLYPHS = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ:!/"
CASES = ("sandbox-eye", "normal-eye", "sandbox-colony", "normal-colony",
         "normal-after-tutorial")


def checksum(data):
    value = 0x5A
    for byte in data:
        value = (((value << 1) | (value >> 7)) & 255) ^ byte
    return value


def battery_fixture(sandbox, colony):
    # Preserve the shipping format: 18 header bytes, 256 height bytes,
    # 128 food bytes, and fifty three-byte ant entries. PyBoy's MBC5 RAM
    # file spans 8 KiB; the actual game occupies only the first 617 bytes.
    ram = bytearray(8192)
    config = bytearray([0xC5, 1] + [0] * 18)
    ram[:21] = config + bytes([checksum(config)])
    # EASY gives the enemy queen four HP for the crowded-colony test.
    # Seven soldiers share one processing slice: pre-tick healing alone
    # used to allow them to kill a sandbox queen during that slice.
    difficulty, queen_hp = (0, 4) if colony else (1, 5)
    header = bytearray([
        0xA7, difficulty, 3 if sandbox else 0,
        0, 0, 7 if colony else 5, queen_hp, 0, 0,
        26, 5, 20, 50, 0, 0, 0, 0, 0,
    ])
    heights = [0 if colony else 1] * 1024
    heights[26 * 32 + 4] = heights[5 * 32 + 27] = 1
    packed_heights = bytes(
        sum(heights[i + j] << (j * 2) for j in range(4))
        for i in range(0, 1024, 4)
    )
    ants = bytearray(150)
    if colony:
        slots = list(range(0, 50, 8))
        slots += [i for i in range(50) if i not in slots][:25 - len(slots)]
        for index in slots:
            ants[index * 3:index * 3 + 3] = bytes([27, 5, 9])
    else:
        ants[:3] = bytes([26, 5, 1])
    save = header + packed_heights + bytes(128) + ants
    assert len(save) == 552
    ram[64:64 + len(save) + 1] = save + bytes([checksum(save)])
    return io.BytesIO(ram)


class Cartridge:
    def __init__(self, rom, sandbox, colony, out):
        self.out = out
        self.pyboy = PyBoy(str(rom), ram_file=battery_fixture(sandbox, colony),
                           window="null", cgb=True, sound_volume=0)
        self.pyboy.set_emulation_speed(0)
        symbols = rom.with_suffix(".map").read_text()
        self.cursor = []
        for name in ("cx", "cy"):
            match = re.search(r"([0-9A-Fa-f]{8})\s+_" + name + r"\b", symbols)
            assert match, "Missing production cursor symbol: " + name
            address = int(match[1], 16)
            assert 0xC000 <= address < 0xE000
            self.cursor.append(address)

    def tick(self, frames):
        self.pyboy.tick(frames, True, False)

    def press(self, key, frames=18, after=6):
        self.pyboy.button_press(key)
        self.tick(frames)
        self.pyboy.button_release(key)
        self.tick(after)

    def row(self, y):
        # Read glyph IDs, not the attributes in CGB VRAM bank 1.
        bank = self.pyboy.memory[0xFF4F]
        self.pyboy.memory[0xFF4F] = 0
        try:
            start = 0x9C00 + y * 32
            return "".join(GLYPHS[v - 136] if 136 <= v < 176 else "?"
                           for v in self.pyboy.memory[start:start + 20])
        finally:
            self.pyboy.memory[0xFF4F] = bank & 1

    def boot_title(self):
        self.tick(680)
        self.press("start", 3)
        self.tick(300)

    def quit_tutorial(self):
        # Keep the normal battery save untouched while beginning a new
        # tutorial, enter its first live lesson, then quit through the UI.
        # This previously leaked the active lesson into the next match.
        self.press("a", 18, 350)
        assert "WELCOME RULER!" in self.row(1), "Tutorial did not start"
        self.press("start", 18, 100)
        assert "1/8 THE CURSOR" in self.row(1), "Cursor lesson card missing"
        self.press("start", 18, 100)
        assert self.pyboy.memory[0xFF4A] == 128, "Live lesson did not begin"
        self.snapshot("tutorial-live-lesson")
        self.press("start", 18, 40)
        assert self.row(2).startswith("P"), "Tutorial pause did not open"
        self.press("b", 18, 40)
        assert "B AGAIN: QUIT" in self.row(16), "Tutorial quit confirmation missing"
        self.press("b", 18, 350)
        assert "UP:CONTINUE" in self.row(10), "Tutorial quit lost the normal save"

    def load_save(self, enter_eye, after_tutorial=False):
        self.boot_title()
        if after_tutorial:
            self.quit_tutorial()
        assert "UP:CONTINUE" in self.row(10), "Original-format save rejected"
        self.press("up", 3, 0)
        # Catch the saved cursor during initialization, before the first
        # simulation slice can move our adjacent worker away from the queen.
        for _ in range(300):
            if tuple(self.pyboy.memory[a] for a in self.cursor) == (26, 5):
                break
            self.tick(1)
        else:
            raise AssertionError("Saved world failed to load")
        if enter_eye:
            self.pyboy.button_press("select")
            self.pyboy.button_press("start")
            self.tick(100)
            self.pyboy.button_release("start")
            self.pyboy.button_release("select")
            self.tick(240)
            assert self.row(0).startswith("ANT EYE"), "ANT EYE entry failed"
            assert "RED QUEEN" in self.row(1), "Fixture must face the real queen"
        else:
            self.tick(700)

    def snapshot(self, name):
        self.pyboy.screen.image.save(self.out / (name + ".png"))

    def battery(self):
        # Bank-indexed API reads the save without enabling RAM or changing
        # the game's selected bank. No fixture or state is written here.
        return bytes(self.pyboy.memory[0, 0xA000:0xA000 + 617])


def run_case(rom, name, out):
    sandbox, colony = name.startswith("sandbox"), name.endswith("colony")
    cart = Cartridge(rom, sandbox, colony, out)
    record = {"case": name, "result": "RUNNING", "bites": []}
    try:
        cart.load_save(not colony, name == "normal-after-tutorial")
        cart.snapshot(name + "-before")
        if not colony:
            for index in range(7 if sandbox else 5):
                cart.press("b", 60, 180)
                record["bites"].append({"number": index + 1,
                                        "header": cart.row(0),
                                        "ahead": cart.row(1),
                                        "feedback": cart.row(14)})
                if "YOU WIN" in cart.row(1):
                    break
        cart.snapshot(name + "-after")
        record["final_rows"] = [cart.row(i) for i in (0, 1, 14)]
        if sandbox:
            assert "YOU WIN" not in cart.row(1), "Sandbox ended in victory"
            if not colony:
                assert cart.row(0).startswith("ANT EYE"), "Sandbox left ANT EYE"
                assert "RED QUEEN" in cart.row(1), "Protected queen disappeared"
                cart.press("start", 60, 180)
            assert cart.pyboy.memory[0xFF4A] == 128, "Strategy HUD did not return"
            cart.press("start", 18, 40)
            cart.press("a", 18, 40)
            assert "GAME SAVED!" in cart.row(16), "Surviving game cannot be saved"
            battery = cart.battery()
            block = battery[64:616]
            assert block[0] == 0xA7 and block[2] & 2, "Sandbox save flags changed"
            assert checksum(block) == battery[616], "Surviving save checksum failed"
            assert block[5] > 0 and block[6] > 0, "Saved queen is dead"
            record["saved_queen_hp"] = [block[5], block[6]]
            record["save_roundtrip"] = "original format/checksum retained"
        else:
            assert "YOU WIN! PRESS START" == cart.row(1), "Normal victory was disabled"
            assert cart.battery()[64] == 0, "Finished match retained its consumed save"
            record["normal_victory"] = True
        record["result"] = "PASS"
    except Exception as error:
        cart.snapshot(name + "-failure")
        record["result"] = "FAIL"
        record["error"] = str(error)
    finally:
        cart.pyboy.stop(save=False)
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path)
    parser.add_argument("--out", type=Path, default=Path("build/queen-safety"))
    parser.add_argument("--case", choices=("all",) + CASES, default="all")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    cases = CASES if args.case == "all" else (args.case,)
    results = [run_case(args.rom, name, args.out) for name in cases]
    report = {"rom": str(args.rom),
              "rom_sha256": hashlib.sha256(args.rom.read_bytes()).hexdigest(),
              "map_sha256": hashlib.sha256(args.rom.with_suffix(".map").read_bytes()).hexdigest(),
              "cases": results,
              "result": "PASS" if all(r["result"] == "PASS" for r in results) else "FAIL"}
    (args.out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    return 0 if report["result"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
