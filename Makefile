GBDK_HOME ?= /opt/gbdk
LCC = $(GBDK_HOME)/bin/lcc
# dippinn_logo_data.c is #included by dippinn_logo.c (same ROM bank), so it is not compiled on its own
SRC := $(filter-out src/dippinn_logo_data.c,$(wildcard src/*.c))
HDR := $(wildcard src/*.h) src/dippinn_logo_data.c
all: build/empire-ants.gbc
build/empire-ants.gbc: $(SRC) $(HDR)
	mkdir -p build
	$(LCC) -Wf--opt-code-size -autobank -Wb-min=2 -Wl-m -Wl-j -Wm-yc -Wm-yt0x1B -Wm-ya1 -Wm-yoA -Wm-yn"EMPIRE-ANTS" -o $@ $(SRC)
clean:
	rm -rf build
