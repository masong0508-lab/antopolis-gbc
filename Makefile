GBDK_HOME ?= /opt/gbdk
LCC = $(GBDK_HOME)/bin/lcc
LCCFLAGS ?=
# Never permit the GBDK banked-call trampoline (_HOME) to spill into
# switchable ROM1. The old 32KB monolithic layout placed _HOME at 0x7C3D:
# BANKED calls switched away the trampoline mid-call and shut down the LCD.
# GBDK CRT0 uses ROM 0x150..0x1FF for its actual startup code,
# not merely the 0x100..0x14F Nintendo header. Never put _HOME there.
# Place the banked-call trampolines at 0x200. Their verified end
# is 0x41F; fixed CODE begins at 0x4C0, leaving 161 bytes of safety
# space between areas while reclaiming 48 unused ROM bytes for usability.
# CI verifies neither area overlaps the other or crosses ROM bank 0.
BANKFIX = -Wl-b_HOME=0x0200 -Wl-b_CODE=0x04C0
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
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_terrain_food.c -o build/test_terrain_food
	./build/test_terrain_food
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_eye_navigation.c -o build/test_eye_navigation
	./build/test_eye_navigation
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_eye_occupancy.c -o build/test_eye_occupancy
	./build/test_eye_occupancy
	python3 tools/test_eye_occupancy_integration.py
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_eye_ui.c -o build/test_eye_ui
	./build/test_eye_ui
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_help_paging.c -o build/test_help_paging
	./build/test_help_paging
	python3 tools/test_title_help_wiring.py
	python3 tools/test_help_text.py
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_terrain_build.c -o build/test_terrain_build
	./build/test_terrain_build
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_sprite_logic.c -o build/test_sprite_logic
	./build/test_sprite_logic
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_eye_surface_detail.c -o build/test_eye_surface_detail
	./build/test_eye_surface_detail
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_colony_route.c -o build/test_colony_route
	./build/test_colony_route
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_colony_defense.c -o build/test_colony_defense
	./build/test_colony_defense
	python3 tools/test_colony_defense_source.py
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_colony_cursor.c -o build/test_colony_cursor
	./build/test_colony_cursor
	python3 tools/test_colony_cursor_wiring.py
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_colony_milestones.c -o build/test_colony_milestones
	./build/test_colony_milestones
	python3 tools/test_colony_route_source.py
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_results_calc.c -o build/test_results_calc
	./build/test_results_calc
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_perk_labels.c -o build/test_perk_labels
	./build/test_perk_labels
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_rules_calc.c -o build/test_rules_calc
	./build/test_rules_calc
	$(HOSTCC) -std=c99 -Wall -Wextra -Werror -pedantic tools/test_palette_fade.c -o build/test_palette_fade
	./build/test_palette_fade
