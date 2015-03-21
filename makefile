all: 
	gcc -c chord.c && gcc -c csapp.c
	gcc -pthread csapp.o chord.o -o chord