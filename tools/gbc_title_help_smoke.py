#!/usr/bin/env python3
"""Actual GBC ROM walkthrough of visible title help and line/page navigation.

Uses a legal battery file only at boot and ordinary joypad input.
No game RAM or ROM patching. Saves screenshots and compact JSON evidence.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
from gbc_queen_safety_smoke import Cartridge

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom",type=Path)
    parser.add_argument("--out",type=Path,default=Path("build/title-help"))
    args=parser.parse_args()
    args.out.mkdir(parents=True,exist_ok=True)
    cart=Cartridge(args.rom,False,False,args.out)
    report={"rom_sha256":hashlib.sha256(args.rom.read_bytes()).hexdigest(),
            "checks":[],"passed":False}
    try:
        cart.boot_title()
        title9=cart.row(9)
        title13=cart.row(13)
        assert "START: PLAY" in title9, ("Normal-play control hidden",title9)
        assert "GOAL: FEED YOUR ANTS" in title13, ("Opening goal hidden",title13)
        cart.snapshot("01_title_quickstart")
        report["checks"].append("Normal play and first objective visible on title")
        cart.press("start",18,550)
        cart.press("start",18,85)
        assert "COLONY COMMAND" in cart.row(0), ("START did not pause",cart.row(0))
        footer=cart.row(17)
        assert "START:GO" in footer and "L/R PG" in footer, footer
        initial=cart.row(3)
        cart.snapshot("02_help_first_page")
        cart.press("right",3,20)
        next_page=cart.row(3)
        assert next_page != initial, ("RIGHT did not move a help page",initial,next_page)
        cart.snapshot("03_help_next_page")
        cart.press("left",3,20)
        assert cart.row(3)==initial, "LEFT did not restore the previous help page"
        report["checks"].append("LEFT/RIGHT jump help pages and preserve original content")
        cart.press("down",3,20)
        assert cart.row(3)!=initial, "DOWN did not move one help line"
        cart.press("up",3,20)
        assert cart.row(3)==initial, "UP did not reverse a line scroll"
        report["checks"].append("UP/DOWN still scroll a single help line")
        cart.press("start",18,70)
        assert "COLONY COMMAND" not in cart.row(0), "START did not return to game"
        cart.snapshot("04_resumed_colony")
        report["checks"].append("START resumes the same playable colony")
        # Read the real compiled SRAM/world symbols; the cartridge receives
        # only ordinary GBC buttons, never modified RAM or injected code.
        symbols=args.rom.with_suffix(".map").read_text()
        def addr(name):
            match=re.search(r"([0-9A-Fa-f]{8})\s+_" + name + r"\b",symbols)
            assert match, "Missing shipping symbol: " + name
            value=int(match[1],16)
            assert 0xC000 <= value < 0xE000
            return value
        mem=cart.pyboy.memory
        cx,cy=addr("cx"),addr("cy")
        nx,ny=addr("nestx"),addr("nesty")
        hgt=addr("hgt")
        own=(mem[nx],mem[ny])
        rival=(mem[nx+1],mem[ny+1])
        assert own != rival and all(0 <= n < 32 for n in own + rival)
        before=bytes(mem[hgt:hgt+1024])
        def jump_and_check(key,target,shot):
            cart.pyboy.button_press("select")
            cart.pyboy.button_press(key)
            cart.tick(20)
            assert (mem[cx],mem[cy]) == target, (
                "SELECT nest shortcut did not recenter",key,target,(mem[cx],mem[cy]))
            # A held chord must not keep moving one tile per repeat frame.
            cart.tick(40)
            assert (mem[cx],mem[cy]) == target, (
                "Holding SELECT moved cursor off its nest",key)
            cart.pyboy.button_release(key)
            cart.pyboy.button_release("select")
            cart.tick(12)
            assert (mem[cx],mem[cy]) == target, "Release moved cursor"
            cart.snapshot(shot)
        jump_and_check("up",own,"05_warp_home")
        jump_and_check("down",rival,"06_warp_rival")
        assert bytes(mem[hgt:hgt+1024]) == before, (
            "Nest shortcuts modified terrain: possible SELECT-release flood")
        # A displayed emulator frame is not one game-loop input poll:
        # painting/simulation spans multiple VBlanks. Validate that the
        # released SELECT restores ordinary scrolling without overshoot.
        # At most six moves are possible in 24 consecutive input polls.
        x0,y0=mem[cx],mem[cy]
        direction="left" if x0>=16 else "right"
        cart.press(direction,24,8)
        x1,y1=mem[cx],mem[cy]
        steps=x0-x1 if direction=="left" else x1-x0
        assert y1==y0 and 1 <= steps <= 6, (
            "Cursor movement direction or repeat upper bound changed",
            direction,(x0,y0),(x1,y1),steps)
        cart.snapshot("07_cursor_repeat")
        report["checks"].append(
            "Cursor resumes in the intended direction with bounded repetition")
        assert bytes(mem[hgt:hgt+1024]) == before, (
            "Nest shortcuts modified terrain: possible SELECT-release flood")
        report["checks"].append(
            "SELECT+UP/DOWN reach both real nests; held/released chord never floods")
        report["nest_warps"]={"home":own,"rival":rival}
        report["passed"]=True
    except Exception as exc:
        report["error"]=str(exc)
        cart.snapshot("failure")
    finally:
        report["emulated_frames"]=cart.pyboy.frame_count
        cart.pyboy.stop(save=False)
        (args.out/"report.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report),flush=True)
    return 0 if report["passed"] else 1

if __name__=="__main__":
    raise SystemExit(main())
