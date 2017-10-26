CC=clang
CFLAGS=-Wall $(shell pkg-config --cflags --libs libnotify)

all: rossa

debug: CFLAGS += -g
debug: all

config.h:
	@echo Creating config.h from config.def.h
	@cp config.def.h $@

rossa: rossa.c systemd_action.c config.h
	$(CC) $(CFLAGS) rossa.c systemd_action.c -o rossa

clean:
	rm rossa

install: rossa
	install rossa ${HOME}/bin
