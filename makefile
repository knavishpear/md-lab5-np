CC = gcc
CFLAGS = -Wall

all: drone5

drone5: drone5.c
	$(CC) $(CFLAGS) drone5.c -o drone5 -lm

clean:
	rm -f drone5
