all: mbr

mbr: readwrite.o mbr.o superblock.o main.o utils.o pass1.o pass2.o pass3.o pass4.o bitmaps.o
	gcc -o myfsck mbr.o readwrite.o superblock.o pass1.o pass2.o pass3.o pass4.o main.o utils.o bitmaps.o -lm

readwrite.o: readwrite.c
		 gcc -c  -ggdb readwrite.c

bitmaps.o: bitmaps.c
		 gcc -c  -ggdb bitmaps.c

pass1.o: pass1.c
		 gcc -c -ggdb pass1.c

pass2.o: pass2.c
		 gcc -c -ggdb pass2.c

pass3.o: pass3.c
		 gcc -c -ggdb pass3.c

pass4.o: pass4.c
		 gcc -c -ggdb pass4.c

utils.o: utils.c
		 gcc -c -ggdb utils.c

mbr.o: mbr.c
		 gcc -c -ggdb mbr.c

superblock.o: superblock.c
		gcc -c -ggdb superblock.c

main.o: main.c
		gcc -c -ggdb main.c

clean:
		 rm mbr.o readwrite.o superblock.o main.o pass1.o utils.o pass2.o pass3.o pass4.o bitmaps.o myfsck
