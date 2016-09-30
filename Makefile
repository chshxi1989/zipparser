all:
	gcc -c zip.c -o zip.o
	gcc -c hashtable.c -o hashtable.o
	gcc zip.o hashtable.o -lz -o zip