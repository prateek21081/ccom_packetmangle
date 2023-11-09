CC=gcc

all: nfqclient.c
	$(CC) -g -o nfqclient.out nfqclient.c -lnetfilter_queue
	$(CC) -g -o nfqserver.out nfqserver.c -lnetfilter_queue

clean:
	rm *.out
