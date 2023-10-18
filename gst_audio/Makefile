CC=gcc
CFLAGS=-g -Wall -Wextra `pkg-config --cflags --libs gstreamer-1.0`

all:
	$(CC) $(CFLAGS) gstaudio.c -o gstaudio

clean:
	rm -f gstaudio
