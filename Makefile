GBDK_HOME ?= /opt/gbdk
LCC = $(GBDK_HOME)/bin/lcc
all: build/empire-ants.gbc
build/empire-ants.gbc: $(wildcard src/*.c) $(wildcard src/*.h)
	mkdir -p build
	$(LCC) -Wf--opt-code-size -Wl-m -Wl-j -Wm-yc -Wm-yt0x1B -Wm-ya1 -Wm-yo4 -Wm-yn"EMPIRE-ANTS" -o $@ src/*.c
clean:
	rm -rf build
