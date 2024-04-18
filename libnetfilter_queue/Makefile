# Makefile

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall

# Find all .c files in the current directory and its subdirectories
SRCS = $(shell find . -name "*.c")

# Define the .out file for each .c file
OUTS = $(SRCS:%.c=%.out)

all: $(OUTS)

# Rule to compile .c files into .out files
%.out: %.c
	$(CC) $(CFLAGS) $< -o $@ -lnetfilter_queue

install:
	@if [ -z "$(dir)" ]; then \
		echo "dir is not set. Use make install dir=<directory>"; \
		exit 1; \
    fi
	@for file in $(shell find ./$(dir) -name "*.out"); do \
		cp $$file /usr/local/bin/`basename $$file .out`; \
	done

uninstall:
	@if [ -z "$(dir)" ]; then \
		echo "dir is not set. Use make uninstall dir=<directory>"; \
		exit 1; \
	fi
	@for file in $(shell find ./$(dir) -name "*.out"); do \
		rm /usr/local/bin/`basename $$file .out`; \
	done

clean:
	find . -name "*.out" -type f -delete
