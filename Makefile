CC=clang
CFLAGS=-Wall $(shell pkg-config --cflags --libs libnotify)

all: rossa

debug: CFLAGS += -g
debug: all

config.h:
	cp config.def.h $@

rossa: rossa.c config.h
	$(CC) $(CFLAGS) rossa.c -o rossa

clean:
	rm rossa

install:
	install rossa ${HOME}/bin
