#!/usr/bin/env python3
"""Protect live GBC nest shortcut controls and original input behaviour."""
from pathlib import Path

source = Path("src/main.c").read_text()
help_text = Path("src/text2.c").read_text()
assert '#include "colony_cursor.h"' in source
pin = source.split("static void pin(uint8_t k, Pad *d) {", 1)[1].split(
    "static void pass_card(void)", 1
)[0]
assert "(k & J_SELECT) && (dirs & (J_UP | J_DOWN))" in pin
assert "colony_cursor_target(pl, (uint8_t)((dirs & J_DOWN) >> 3))" in pin
assert "cx = nestx[team]; cy = nesty[team];" in pin
assert "dirs = 0;" in pin, "Held jump must suppress ordinary movement"
assert "d->su = 1;" in pin, "Releasing SELECT must not flood"
assert pin.index("if (p & J_SELECT) d->su = 0;") < pin.index(
    "if ((k & J_SELECT)") < pin.index("d->su = 1;"), (
    "SELECT reset must happen before jump consumes SELECT on the same frame"
)
assert "(d->rp % 3) == 1" in pin, "Keep the original repeat cadence"
assert "if (++d->rp == 252u) d->rp = 12u;" in pin, (
    "Very long holds must preserve repeat phase, not restart the delay"
)
assert "if (d->sl && !d->su) flood();" in pin, "Keep ordinary SELECT flood"
assert "if (p & J_A) { offering();" in pin, "Keep SELECT+A trade"
assert 'SEL UP HOME DN RIVAL' in help_text
print("GBC nest warp wired with no drift/flood; original controls preserved")

# Compile the actual first-person and pause-menu repeat conditions and
# counter updates, keeping the existing delay and four-poll cadence.
import re
import os
import shlex
import subprocess
import tempfile

eye = source.split("static void eye(void) {", 1)[1].split(
    "// ---------- TUTORIAL:", 1
)[0]
play = source.split("static void play(void) {", 1)[1]
for section, delay in ((eye, 8), (play, 12)):
    condition = re.search(r"if \(rep == 0 \|\| \(rep >= " + str(delay) + r" && !\(rep & 3\)\)\)", section)
    advance = re.search(r"if \(\+\+rep == 252u\) rep = " + str(delay) + r"u;", section)
    assert condition and advance, "Keep original repeat cadence and phase-preserving wrap"
    program = """
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
int main(void) {
  uint8_t rep = 0;
  unsigned moves = 0;
  for(unsigned frame=0; frame<20000; ++frame) {
    unsigned before=moves;
    """ + condition.group() + """ ++moves;
    """ + advance.group() + """
    assert(moves-before == (unsigned)(frame==0 || (frame>=""" + str(delay) + """ && frame%4==0)));
  }
  rep=0;
  """ + condition.group() + """ ++moves;
  assert(rep==0);
  return 0;
}
"""
    with tempfile.TemporaryDirectory(prefix="eye-repeat-") as d:
        p=Path(d)/"test.c"; exe=Path(d)/"test"
        p.write_text(program)
        subprocess.run(shlex.split(os.environ.get("HOSTCC","cc"))+["-std=c99","-Wall","-Wextra","-Werror","-pedantic",str(p),"-o",str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
print("ANT EYE and pause menu hold: 40000 polls, unchanged repeat cadence")

# Test live overhead repeat timing for continuous map movement.
condition = re.search(r"if \(d->rp == 0 \|\| \(d->rp >= 10 && \(d->rp % 3\) == 1\)\)", pin)
advance = re.search(r"if \(\+\+d->rp == 252u\) d->rp = 12u;", pin)
assert condition and advance
program = """
#include <stdint.h>
#include <assert.h>
int main(void) {
  uint8_t rep=0;
  for(unsigned frame=0;frame<20000;frame++){
    unsigned moves=0;
""" + condition.group().replace("d->rp","rep") + """ ++moves;
""" + advance.group().replace("d->rp","rep") + """
    assert(moves==(frame==0||(frame>=10&&frame%3==1)));
  }
  return 0;
}
"""
with tempfile.TemporaryDirectory(prefix="map-repeat-") as d:
    p=Path(d)/"test.c";exe=Path(d)/"test";p.write_text(program)
    subprocess.run(shlex.split(os.environ.get("HOSTCC","cc"))+
        ["-std=c99","-Wall","-Wextra","-Werror","-pedantic",str(p),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print("GBC overhead cursor: 20000 polls without repeat-delay resets")
