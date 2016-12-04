CC=clang
CFLAGS=-Wall $(shell pkg-config --cflags --libs libnotify)

all: rossa

debug: CFLAGS += -g
debug: all

rossa: rossa.c
	$(CC) $(CFLAGS) rossa.c -o rossa

clean:
	rm rossa
