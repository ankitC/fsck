all: myfsck

myfsck: readwrite.o myfsck.o superblock.o balloc.o main.o
		 gcc -o myfsck myfsck.o readwrite.o superblock.o main.o

readwrite.o: readwrite.c
		 gcc -c  -ggdb readwrite.c

balloc.o: balloc.c
		 gcc -c -ggdb balloc.c


myfsck.o: myfsck.c
		 gcc -c -ggdb myfsck.c

superblock.o: superblock.c
		gcc -c -ggdb superblock.c

main.o: main.c
		gcc -c -ggdb main.c

clean:
		 rm myfsck.o readwrite.o superblock.o main.o balloc.o myfsck
