CC=clang
CFLAGS=-Wall

all: rossa

debug: CFLAGS += -g
debug: all

rossa: rossa.c
	$(CC) $(CFLAGS) rossa.c -o rossa

clean:
	rm rossa
