CC=gcc
CFLAGS=-W -Wall -pedantic -pedantic-errors -O9 -Winline

all: index
index: SAc.o ds/ds.o ds/globals.o ds/shallow.o ds/helped.o ds/deep2.o ds/blind2.o
	ar rc SAc.a SAc.o ds/ds.o ds/globals.o ds/shallow.o ds/helped.o ds/deep2.o ds/blind2.o
clean:
	rm -f *~ *.o ; cd ds ; make -f Makefile clean ; cd ..
cleanall:
	rm -f *~ *.o *.a; cd ds ; make -f Makefile cleanall ; cd ..
tarfile:
	tar zcvf SAc.tgz ?akefile *.c *.h ds README COPYRIGHT

# pattern rule for all objects files
%.o: %.c *.h
	$(CC) -c $(CFLAGS) $< -o $@

