CC=gcc

all: nf-queue.c
	$(CC) -g3 -ggdb -Wall -lmnl -lnetfilter_queue -o nf-queue.o nf-queue.c
