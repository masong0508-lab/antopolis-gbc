GBDK_HOME ?= /opt/gbdk
LCC = $(GBDK_HOME)/bin/lcc
SRC := $(wildcard src/*.c)
HDR := $(wildcard src/*.h)
all: build/empire-ants.gbc
build/empire-ants.gbc: $(SRC) $(HDR)
	mkdir -p build
	$(LCC) -Wm-yc -Wm-yn"EMPIRE-ANTS" -o $@ $(SRC)
clean:
	rm -rf build
