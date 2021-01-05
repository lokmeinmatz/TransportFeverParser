CC = gcc
CFLAGS = -g -Wall

build: src/main.c
	$(CC) $(CFLAGS) -o main src/main.c -llz4 
