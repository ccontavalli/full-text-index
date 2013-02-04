SHELL=/bin/sh

CC=gcc

# read options (such as -march) which are specific
# for a particular machine
#LOCAL_OPTIONS=`cat local_options`
LOCAL_OPTIONS=-march=pentium4

#these are for testing
CFLAGS = -g -W -Wall -O2 -Winline

#these are for debugging
#CFLAGS = -g -W -Wall

#these are for maximum speed
#CFLAGS=-g -O3 -fomit-frame-pointer -W -Wall -Winline \
#       -DDEBUG=0 -DNDEBUG=1 $(LOCAL_OPTIONS)

# these are for profiling (-a?) 
#CFLAGS = -g -pg -O2 -W -Wall -Winline -DDEBUG=0


.PHONY: all
all : bwtopt

# bwtopt algorithm
bwtopt: bwtopt.c bwtopt1.c 
	$(CC) $(CFLAGS) -o bwtopt bwtopt.c bwtopt1.c bwtlcp.a ds_ssort.a -lm


# pattern rule for all objects files
%.o: %.c *.h
	$(CC) -c $(CFLAGS) $< -o $@

clean: 
	rm -f *.o  bwtopt

cleanall: clean
	rm -f *.a

tarfile:
	tar zcvf bwtopt.tgz ?akefile *.c *.a *.h 
