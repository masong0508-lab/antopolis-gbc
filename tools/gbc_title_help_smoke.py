#!/usr/bin/env python3
"""Actual GBC ROM walkthrough of visible title help and line/page navigation.

Uses a legal battery file only at boot and ordinary joypad input.
No game RAM or ROM patching. Saves screenshots and compact JSON evidence.
"""
import argparse
import hashlib
import json
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
