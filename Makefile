CFLAGS=-std=gnu11 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -g -O0 -lpthread
CC=gcc
default: all
all: app clip

app: src/app.c
	$(CC) -o application src/app.c src/library.c src/communication.c src/protocol.c $(CFLAGS)

clip: src/clip.c
	$(CC) -o clipboard src/clip.c src/communication.c src/threads.c src/protocol.c $(CFLAGS)

clean:
	-rm -rf application clipboard CLIPBOARD_SOCKET clip1/* clip2/* clip3/* clip4/*

multi-clip:
	mkdir -p clip1
	mkdir -p clip2
	mkdir -p clip3
	mkdir -p clip4
	cp clipboard clip1
	cp clipboard clip2
	cp clipboard clip3
	cp clipboard clip4