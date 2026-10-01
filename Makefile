GBDK_HOME ?= /opt/gbdk
LCC = $(GBDK_HOME)/bin/lcc
all: build/empire-ants.gbc
build/empire-ants.gbc: src/main.c
	mkdir -p build
	$(LCC) -Wm-yc -Wm-yn"EMPIRE-ANTS" -o $@ $<
clean:
	rm -rf build
