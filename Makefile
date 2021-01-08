CC = gcc
INC = /usr/local/include

build:
	$(CC) *.c -I/$(INC) -lzip -o snesmov
