CC=gcc

DSDIR=../../ds
DSFILES=$(DSDIR)/ds.o $(DSDIR)/globals.o $(DSDIR)/helped.o $(DSDIR)/shallow.o $(DSDIR)/deep2.o $(DSDIR)/blind2.o 

#CFLAGS = -g -lm -pg -fprofile-arcs -O9 -W -Wall -Winline -DDEBUG=0
#these are for testing
#CFLAGS = -g -lm -W -Wall -Winline -O9

#these are for maximum speed
CFLAGS=-g -O3 -lm -fomit-frame-pointer -W -Wall -Winline -DDEBUG=0 -DNDEBUG=1 -I../../

all: fm.a fm_search fm_build

fm_build:	fm_build_main.c fm.a
	$(CC) $(CFLAGS) -o fm_build fm_build_main.c fm.a

fm_search:	fm_search_main.c fm.a 
	$(CC) $(CFLAGS) -o fm_search fm_search_main.c fm.a
	
example:	build_index_Example.c fm.a
	$(CC) $(CFLAGS) -o bexample build_index_Example.c fm.a
	
# archive containing fm-library
fm.a: fm_mng_bits.o fm_common.o fm_search.o fm_errors.o fm_read.o fm_occurences.o fm_multihuf.o fm_huffman.o fm_extract.o fm_build.o $(DSFILES)
	ar rc fm.a fm_common.o fm_mng_bits.o fm_search.o fm_build.o fm_errors.o fm_read.o fm_occurences.o fm_multihuf.o fm_huffman.o fm_extract.o $(DSFILES)
	
fm_build.o:	fm_build.c fm_common.o fm_mng_bits.o fm_multihuf.o fm_huffman.o fm_occurences.o 

fm_search.o:	fm_search.c fm_mng_bits.o fm_common.o fm_occurences.o

fm_read.o:	fm_read.c fm_mng_bits.o fm_common.o

fm_multihuf.o:	fm_multihuf.c fm_huffman.o fm_mng_bits.o

fm_huffman.o:	fm_huffman.c

fm_errors.o:	fm_errors.c 

fm_mng_bits.o:	fm_mng_bits.c fm_common.o

fm_common.o:	fm_common.c 

fm_occurences.o:	fm_occurences.c fm_mng_bits.o

fm_extract.o:	fm_extract.c fm_mng_bits.o

clean:	
	make -C $(DSDIR) clean
	rm -f *.o  *~ fm_build fm_search

cleanall: clean
	make -C $(DSDIR) cleanall
	rm -f *.a
	
release:	
	make clean; rm -f fm_search fm_build fmi_qshell fmi_bshell bexample ds_ssort/ds ds_ssort/bwt ds_ssort/unbwt ds_ssort/testlcp;
