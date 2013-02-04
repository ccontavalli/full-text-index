CC = gcc
CFLAGS = -O3
#CFLAGS = -O3 -m64 -DBIT64

csa-dna.a: comparray5.o core.o comparray5.h typedef.h interface.o interface.h qsufsort.o
	ar rc csa-dna.a interface.o comparray5.o core.o qsufsort.o

mkcsa: mkarray2.c qsufsort.c comparray5.c core.c comparray5.h typedef.h
	$(CC)  $(CFLAGS) -o mkcsa mkarray2.c qsufsort.c comparray5.c core.c
csatest: suftest4.c comparray5.c core.c comparray5.h typedef.h
	$(CC)  $(CFLAGS) -o csatest suftest4.c comparray5.c core.c

clean:
	rm -f *.o

cleanall: clean
	rm -f *.a

