CC=g++
CFLAGS=-W -Wall -pedantic -pedantic-errors -O9 -DNDEBUG -I../../

DSDIR=../../ds
DSFILES=$(DSDIR)/ds.o $(DSDIR)/globals.o $(DSDIR)/helped.o $(DSDIR)/shallow.o $(DSDIR)/deep2.o $(DSDIR)/blind2.o 

all: af.a

# pattern rule for all objects files
%.o: %.c *.h
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cpp *.h
	$(CC) -c $(CFLAGS) $< -o $@

test: test.o af.a
	$(CC) $(CFLAGS) test.o af.a -o $@

af.a: af-index.o bitarray.o bitrankw32int.o HuffAlphabetRank.o Huffman_Codes.o $(DSFILES) bwtopt/lcp_aux.o bwtopt/bwt_aux.o bwtopt/bwtopt1.o
	ar rc af.a af-index.o bitarray.o bitrankw32int.o HuffAlphabetRank.o Huffman_Codes.o $(DSFILES) bwtopt/lcp_aux.o bwtopt/bwt_aux.o bwtopt/bwtopt1.o

clean:
	rm -f *~ *.o test; ( cd $(DSDIR) ; make -f Makefile clean ); (cd bwtopt ; make -f Makefile clean) ; cd ..
cleanall:
	rm -f *~ *.o *.a test; ( cd $(DSDIR) ; make -f Makefile cleanall ) ; (cd bwtopt ; make -f Makefile cleanall) ; cd .. 

tarfile:
	tar zcvf af-index_v2.1.tgz ?akefile *.c *.h *.cpp $(DSDIR) bwtopt COPYRIGHT README
