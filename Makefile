all:
	gcc -c zip.c -o zip.o
	gcc zip.o -lz -o zip
