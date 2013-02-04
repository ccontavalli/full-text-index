CC=gcc
CFLAGS=-O9 -DNDEBUG -I../../

all: csa-sada.a

suftest3.test.o: suftest3.test.c
	$(CC) $(CFLAGS) -c suftest3.test.c
comparray4.o: comparray4.c
	$(CC) $(CFLAGS) -c comparray4.c
qsufsort.o: qsufsort.c
	$(CC) $(CFLAGS) -c qsufsort.c
mmap.o: mmap.c
	$(CC) $(CFLAGS) -c mmap.c

csa-sada.a: suftest3.test.o comparray4.o mmap.o qsufsort.o
	ar rc csa-sada.a suftest3.test.o comparray4.o mmap.o qsufsort.o

clean:
	-rm -f *~ *.o *.bak 
cleanall: clean
	-rm -f *.a

tarfile:
	tar zcvf sada_csa.tgz ?akefile 386.c mman.h suftest3.test.* comparray4.* qsufsort.* mmap.* COPYRIGHT README
