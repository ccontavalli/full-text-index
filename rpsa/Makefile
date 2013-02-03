CC=g++
CFLAGS= -W -Wall -pedantic -pedantic-errors -O3 -DNDEBUG
#CFLAGS= -g

# pattern rule for all objects files
%.o: %.c *.h
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cpp *.h
	$(CC) -c $(CFLAGS) $< -o $@

all: RPSA.a

redo: cleanall all

RPSA.a: SA_BPE.o bpe.o bitrankw32int.o mylist.o Treap.o ds/ds.o ds/globals.o ds/shallow.o ds/helped.o ds/deep2.o ds/blind2.o
	ar rc RPSA.a SA_BPE.o bpe.o bitrankw32int.o mylist.o Treap.o ds/ds.o ds/globals.o ds/shallow.o ds/helped.o ds/deep2.o ds/blind2.o
clean:
	rm -f *~ *.o ; cd ds ; make -f Makefile clean ; cd ..
cleanall:
	rm -f *~ *.o *.a main; cd ds ; make -f Makefile cleanall ; cd ..
tarfile:
	tar zcvf RPSA.tgz ?akefile *.cpp *.h ds README COPYRIGHT

