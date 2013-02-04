CC=g++
CFLAGS= -m32 -W -Wall -pedantic -pedantic-errors -O3 -DNDEBUG -I../../

DSDIR=../../ds
DSFILES=$(DSDIR)/ds.o $(DSDIR)/globals.o $(DSDIR)/helped.o $(DSDIR)/shallow.o $(DSDIR)/deep2.o $(DSDIR)/blind2.o 

# pattern rule for all objects files
%.o: %.c *.h
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cpp *.h
	$(CC) -c $(CFLAGS) $< -o $@

all: lcsa.a

redo: cleanall all

lcsa.a: SA_BPE.o bpe.o bitrankw32int.o mylist.o Treap.o $(DSFILES)
	ar rc lcsa.a SA_BPE.o bpe.o bitrankw32int.o mylist.o Treap.o $(DSFILES)
clean:
	rm -f *~ *.o ; cd $(DSDIR) ; make -f Makefile clean ; cd ..
cleanall:
	rm -f *~ *.o *.a main; cd $(DSDIR) ; make -f Makefile cleanall ; cd ..
tarfile:
	tar zcvf LCSA2.tgz ?akefile *.cpp *.h ds README COPYRIGHT

