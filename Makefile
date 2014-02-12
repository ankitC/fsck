all: myfsck

myfsck: readwrite.o myfsck.o
		 gcc -o myfsck myfsck.o readwrite.o

readwrite.o: readwrite.c
		 gcc -c  readwrite.c

myfsck.o: myfsck.c
		 gcc -c  myfsck.c

clean:
		 rm myfsck.o readwrite.o myfsck
