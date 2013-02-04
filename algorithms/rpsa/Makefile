CC=g++
CFLAGS= -W -Wall -pedantic -pedantic-errors -O3 -DNDEBUG -I../../

DSDIR=../../ds
DSFILES=$(DSDIR)/ds.o $(DSDIR)/globals.o $(DSDIR)/helped.o $(DSDIR)/shallow.o $(DSDIR)/deep2.o $(DSDIR)/blind2.o 

# pattern rule for all objects files
%.o: %.c *.h
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cpp *.h
	$(CC) -c $(CFLAGS) $< -o $@

all: rpsa.a

redo: cleanall all

rpsa.a: SA_BPE.o bpe.o bitrankw32int.o mylist.o Treap.o $(DSFILES)
	ar rc rpsa.a SA_BPE.o bpe.o bitrankw32int.o mylist.o Treap.o $(DSFILES)
clean:
	rm -f *~ *.o ; cd $(DSDIR) ; make -f Makefile clean ; cd ..
cleanall:
	rm -f *~ *.o *.a main; cd $(DSDIR) ; make -f Makefile cleanall ; cd ..
tarfile:
	tar zcvf RPSA.tgz ?akefile *.cpp *.h ds README COPYRIGHT

