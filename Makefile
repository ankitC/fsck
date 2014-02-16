all: mbr

mbr: readwrite.o mbr.o superblock.o balloc.o main.o
		 gcc -o mbr mbr.o readwrite.o superblock.o main.o

readwrite.o: readwrite.c
		 gcc -c  -ggdb readwrite.c

balloc.o: balloc.c
		 gcc -c -ggdb balloc.c


mbr.o: mbr.c
		 gcc -c -ggdb mbr.c

superblock.o: superblock.c
		gcc -c -ggdb superblock.c

main.o: main.c
		gcc -c -ggdb main.c

clean:
		 rm mbr.o readwrite.o superblock.o main.o balloc.o mbr
