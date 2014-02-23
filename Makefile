all: mbr

mbr: readwrite.o mbr.o superblock.o balloc.o main.o utils.o pass1.o 
		 gcc -o myfsck mbr.o readwrite.o superblock.o pass1.o main.o utils.o

readwrite.o: readwrite.c
		 gcc -c  -ggdb readwrite.c

balloc.o: balloc.c
		 gcc -c -ggdb balloc.c

pass1.o: pass1.c
		 gcc -c -ggdb pass1.c

utils.o: utils.c
		 gcc -c -ggdb utils.c

mbr.o: mbr.c
		 gcc -c -ggdb mbr.c

superblock.o: superblock.c
		gcc -c -ggdb superblock.c

main.o: main.c
		gcc -c -ggdb main.c

clean:
		 rm mbr.o readwrite.o superblock.o main.o balloc.o pass1.o utils.o myfsck
