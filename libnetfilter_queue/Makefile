CC=gcc

all: nfqclient.c
	$(CC) -lnetfilter_queue -o nf-queue.out nf-queue.c

clean:
	rm *.out
