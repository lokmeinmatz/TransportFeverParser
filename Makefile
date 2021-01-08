CC = gcc
CFLAGS = -g -Wall -std=c17

SRC_FILES = $(wildcard src/*.c)

build: src/main.c
	$(CC) $(CFLAGS) -o main $(SRC_FILES) -llz4 
