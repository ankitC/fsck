all: myfsck

myfsck: readwrite.o myfsck.o main.o
		 gcc -o myfsck main.o myfsck.o readwrite.o

readwrite.o: readwrite.c
		 gcc -c  readwrite.c

myfsck.o: myfsck.c
		 gcc -c  myfsck.c

main.o: main.c
		gcc -c main.c

clean:
		 rm myfsck.o readwrite.o main.o myfsck
