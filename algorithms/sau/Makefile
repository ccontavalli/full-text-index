CC=gcc
CFLAGS= -W -Wall -pedantic -pedantic-errors -O9 -DNDEBUG -I../../

DSDIR=../../ds
DSFILES=$(DSDIR)/ds.o $(DSDIR)/globals.o $(DSDIR)/helped.o $(DSDIR)/shallow.o $(DSDIR)/deep2.o $(DSDIR)/blind2.o 

# pattern rule for all objects files
%.o: %.c *.h
	$(CC) -c $(CFLAGS) $< -o $@

all: sau.a
sau.a: SAu.o $(DSFILES)
	ar rc sau.a SAu.o $(DSFILES)
clean:
	rm -f *~ *.o ; cd $(DSDIR) ; make -f Makefile clean ; cd ..
cleanall:
	rm -f *~ *.o *.a; cd $(DSDIR) ; make -f Makefile cleanall ; cd ..
tarfile:
	tar zcvf SAu.tgz ?akefile *.c *.h ds README COPYRIGHT


