# Copyright Oana-Alexandra Titoc 313CAa 2022-2023


# compiler setup
CC=gcc -g
CFLAGS=-Wall -Wextra -std=c99


# define targets
TARGETS=vma

build: $(TARGETS)

vma: main.o vma.o
	$(CC) $(CFLAGS) main.o vma.o -o vma
main.o: main.c vma.h
	$(CC) $(CFLAGS) -c main.c
vma.o: vma.c
	$(CC) $(CFLAGS) -c vma.c

run_vma: vma
	./vma

pack:
	zip -FSr 313CA_TitocOana-Alexandra_Tema1.zip README Makefile *.c *.h

clean:
	rm -f *.o $(TARGETS)

.PHONY: pack clean


