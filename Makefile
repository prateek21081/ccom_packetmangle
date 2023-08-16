CC=gcc

all: nf-queue-new.c nf-queue.c
	$(CC) -g3 -ggdb -Wall -lmnl -lnetfilter_queue -o nf-queue.out nf-queue.c
	$(CC) -g3 -ggdb -Wall -lmnl -lnetfilter_queue -o nf-queue-new.out nf-queue-new.c

clean:
	rm *.out
