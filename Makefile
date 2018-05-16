CFLAGS=-std=gnu11 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -g -O0
CC=gcc
default: all
all: application clipboard

application: src/application.c
	$(CC) -o bin/application src/application.c $(CFLAGS)

clipboard: src/clipboard.c
	$(CC) -o bin/clipboard src/clipboard.c $(CFLAGS)

clean:
	-rm -f bin/application bin/clipboard