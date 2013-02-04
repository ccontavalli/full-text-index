CC=gcc
CFLAGS= -W -Wall -pedantic -O9 -DNDEBUG -I../../

DSDIR=../../ds
DSFILES=$(DSDIR)/ds.o $(DSDIR)/globals.o $(DSDIR)/helped.o $(DSDIR)/shallow.o $(DSDIR)/deep2.o $(DSDIR)/blind2.o 

all: sac.a

sac.a: SAc.o $(DSFILES)
	ar rc sac.a SAc.o $(DSFILES)
clean:
	rm -f *~ *.o ; cd $(DSDIR) ; make -f Makefile clean ; cd ..
cleanall:
	rm -f *~ *.o *.a; cd $(DSDIR) ; make -f Makefile cleanall ; cd ..
tarfile:
	tar zcvf SAc.tgz ?akefile *.c *.h ds README COPYRIGHT

# pattern rule for all objects files
%.o: %.c *.h
	$(CC) -c $(CFLAGS) $< -o $@

