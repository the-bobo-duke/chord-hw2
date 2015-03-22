all:
	gcc -c csapp.c
	gcc -c chord.c
	gcc -lcrypto -pthread csapp.o chord.o -o chord
	