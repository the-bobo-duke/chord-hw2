all: 
	gcc -c chord.c -lcrypto && gcc -c csapp.c
	gcc -pthread csapp.o chord.o -o chord