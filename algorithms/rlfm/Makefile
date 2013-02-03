CC=g++
CFLAGS= -W -Wall -pedantic -pedantic-errors -O9 -DNDEBUG -I../../

DSDIR=../../ds
DSFILES=$(DSDIR)/ds.o $(DSDIR)/globals.o $(DSDIR)/helped.o $(DSDIR)/shallow.o $(DSDIR)/deep2.o $(DSDIR)/blind2.o 

all: index

# pattern rule for all objects files
%.o: %.c *.h
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cpp *.h
	$(CC) -c $(CFLAGS) $< -o $@

index:	RLFM.o bitarray.o HuffAlphabetRank.o Huffman_Codes.o $(DSFILES)
	ar rc RLFM.a RLFM.o bitarray.o HuffAlphabetRank.o Huffman_Codes.o $(DSFILES)
clean:
	rm -f *~ *.o ; cd $(DSDIR) ; make -f Makefile clean ; cd ..
cleanall:
	rm -f *~ *.o *.a; cd $(DSDIR) ; make -f Makefile cleanall ; cd ..
tarfile:
	tar zcvf RLFM_v1.1.tgz ?akefile *.c *.h *.cpp ds README COPYRIGHT

