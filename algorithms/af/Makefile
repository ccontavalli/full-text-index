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

index: af-index.o bitarray.o bitrankw32int.o HuffAlphabetRank.o Huffman_Codes.o $(DSFILES) bwtopt/lcp_aux.o bwtopt/bwt_aux.o bwtopt/bwtopt1.o
	ar rc af-index.a af-index.o bitarray.o bitrankw32int.o HuffAlphabetRank.o Huffman_Codes.o $(DSFILES) bwtopt/lcp_aux.o bwtopt/bwt_aux.o bwtopt/bwtopt1.o

search: af-index.a run_queries.c
	$(CC) -o AF-index_new  run_queries.c af-index.a
	
build: af-index.a build_index.c
	$(CC) -o AF-index_newb  build_index.c af-index.a

clean:
	rm -f *~ *.o ; cd $(DSDIR) ; make -f Makefile clean ; cd .. ; cd bwtopt ; make -f Makefile clean ; cd ..
cleanall:
	rm -f *~ *.o *.a ; cd $(DSDIR) ; make -f Makefile clean ; cd .. ; cd bwtopt ; make -f Makefile clean ; cd .. 
tarfile:
	tar zcvf af-index_v2.1.tgz ?akefile *.c *.h *.cpp $(DSDIR) bwtopt COPYRIGHT README
