CC = gcc
CFLAGS = -Wall -ansi -pedantic -O2 -pthread -lm -std=c99
OBJECTS = ep1.o

ep1 : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o ep1
 
ep1.o : ep1.c
	$(CC) $(CFLAGS) -c ep1.c