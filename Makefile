GBDK_HOME ?= /opt/gbdk
LCC = $(GBDK_HOME)/bin/lcc
LCCFLAGS ?=
# Never permit the GBDK banked-call trampoline (_HOME) to spill into
# switchable ROM1. The old 32KB monolithic layout placed _HOME at 0x7C3D:
# BANKED calls switched away the trampoline mid-call and shut down the LCD.
# Reserve 0x200-0x4FF for _HOME (0x2A6 bytes currently), then _CODE at 0x500.
# Confirm the bank and non-overlap with a map assertion in CI.
BANKFIX = -Wl-b_HOME=0x0200 -Wl-b_CODE=0x0500
all: build/empire-ants.gbc
build/empire-ants.gbc: $(wildcard src/*.c) $(wildcard src/*.h)
	mkdir -p build
	$(LCC) $(LCCFLAGS) $(BANKFIX) -Wf--opt-code-size -Wl-m -Wl-j -Wm-yc -Wm-yt0x1B -Wm-ya1 -Wm-yo4 -Wm-yn"EMPIRE-ANTS" -o $@ src/*.c
clean:
	rm -rf build

HOSTCC ?= cc
.PHONY: test
test:
	mkdir -p build
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_eye_navigation.c -o build/test_eye_navigation
	./build/test_eye_navigation
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_eye_ui.c -o build/test_eye_ui
	./build/test_eye_ui
	python3 tools/test_help_text.py
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_terrain_build.c -o build/test_terrain_build
	./build/test_terrain_build
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_sprite_logic.c -o build/test_sprite_logic
	./build/test_sprite_logic
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_colony_route.c -o build/test_colony_route
	./build/test_colony_route
	python3 tools/test_colony_route_source.py
