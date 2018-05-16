CFLAGS=-std=gnu11 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -g -O0
CC=gcc
default: all
all: app clip

app: src/app.c
	$(CC) -o bin/application src/app.c $(CFLAGS)

clip: src/clip.c src/lib/com.c
	$(CC) -o bin/clipboard src/clip.c src/lib/com.c $(CFLAGS)

clean:
	-rm -f bin/application bin/clipboard