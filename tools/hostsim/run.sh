#!/bin/sh
# Build the host simulator (with ASan/UBSan) and play N maps with an idle player.
# usage: tools/hostsim/run.sh [N=20]
set -e
cd "$(dirname "$0")"
gcc -O1 -fsanitize=address,undefined -DLIMIT=150000 -I. sim.c -o sim
N=${1:-20}; win=0; lose=0; open=0
for i in $(seq 1 "$N"); do
  r=$(./sim 0 0 "$i" | sed -n 's/.*FIRSTEND.*result //p')
  case "$r" in 1) win=$((win+1));; 2) lose=$((lose+1));; *) open=$((open+1));; esac
done
echo "idle player over $N maps: black wins $win, red wins $lose, no result $open"
./sim 1 0 3 >/dev/null && ./sim 2 0 3 >/dev/null && echo "fuzz + full-map camera sweep: no sanitizer errors"
