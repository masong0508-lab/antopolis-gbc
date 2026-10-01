GBDK_HOME ?= /opt/gbdk
LCC = $(GBDK_HOME)/bin/lcc
all: build/antopolis.gbc
build/antopolis.gbc: src/main.c
	mkdir -p build
	$(LCC) -Wm-yc -Wm-yn"ANTOPOLIS" -o $@ $<
clean:
	rm -rf build
