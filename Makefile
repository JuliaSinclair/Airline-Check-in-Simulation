.phony all:
all: ACS

ACS: main.c
	gcc -Wall -pthread main.c -o ACS

.PHONY clean:
clean:
	-rm -rf *.o *.exe
