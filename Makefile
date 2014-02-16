all: myfsck

myfsck: readwrite.o myfsck.o superblock.o balloc.o main.o
		 gcc -o myfsck myfsck.o readwrite.o superblock.o main.o

readwrite.o: readwrite.c
		 gcc -c  readwrite.c

balloc.o: balloc.c
		 gcc -c  balloc.c


myfsck.o: myfsck.c
		 gcc -c  myfsck.c

superblock.o: superblock.c
		gcc -c superblock.c

main.o: main.c
		gcc -c main.c

clean:
		 rm myfsck.o readwrite.o superblock.o main.o myfsck
