CC=gcc

all: nfqclient.c
	$(CC) -g -o nfqclient.out nfqclient.c -lnetfilter_queue
	$(CC) -g -o nfqserver.out nfqserver.c -lnetfilter_queue
	$(CC) -g -o nfqtss.out nfqtss.c -lnetfilter_queue
	$(CC) -g -o nfqtsc.out nfqtsc.c -lnetfilter_queue

install:
	cp nfqserver.out /usr/local/bin/nfqserver
	cp nfqclient.out /usr/local/bin/nfqclient

clean:
	rm *.out
