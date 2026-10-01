GBDK_HOME ?= /opt/gbdk
LCC = $(GBDK_HOME)/bin/lcc
SRC := $(wildcard src/*.c)
HDR := $(wildcard src/*.h)
all: build/empire-ants.gbc
build/empire-ants.gbc: $(SRC) $(HDR)
	mkdir -p build
	$(LCC) -Wm-yc -Wm-yt0x1B -Wm-ya1 -Wm-yo4 -Wm-yn"EMPIRE-ANTS" -o $@ $(SRC)
clean:
	rm -rf build
