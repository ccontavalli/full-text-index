CC=g++
CFLAGS= -W -Wall -pedantic -pedantic-errors -O9 -DNDEBUG

all: index

# pattern rule for all objects files
%.o: %.c *.h
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cpp *.h
	$(CC) -c $(CFLAGS) $< -o $@

index:	RLFM.o bitarray.o HuffAlphabetRank.o Huffman_Codes.o ds/ds.o ds/globals.o ds/helped.o ds/shallow.o ds/deep2.o ds/blind2.o
	ar rc RLFM.a RLFM.o bitarray.o HuffAlphabetRank.o Huffman_Codes.o ds/ds.o ds/globals.o ds/helped.o ds/shallow.o ds/deep2.o ds/blind2.o
clean:
	rm -f *~ *.o ; cd ds ; make -f Makefile clean ; cd ..
cleanall:
	rm -f *~ *.o *.a; cd ds ; make -f Makefile cleanall ; cd ..
tarfile:
	tar zcvf RLFM_v1.1.tgz ?akefile *.c *.h *.cpp ds README COPYRIGHT

