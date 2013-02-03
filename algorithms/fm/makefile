SHELL=/bin/sh

CC=gcc

#CFLAGS = -g -lm -pg -fprofile-arcs -O9 -W -Wall -Winline -DDEBUG=0
#these are for testing
#CFLAGS = -g -lm -W -Wall -Winline -O9

#these are for maximum speed
CFLAGS=-g -O3 -lm -fomit-frame-pointer -W -Wall -Winline -DDEBUG=0 -DNDEBUG=1 

all: ds_ssortr fm_index.a fm_search fm_build
pizzachili: all build_index run_queries

fm_build:	fm_build_main.c fm_index.a ds_ssortr
	$(CC) $(CFLAGS) -o fm_build fm_build_main.c fm_index.a

fm_search:	fm_search_main.c fm_index.a 
	$(CC) $(CFLAGS) -o fm_search fm_search_main.c fm_index.a
	
example:	build_index_Example.c fm_index.a ds_ssortr
	$(CC) $(CFLAGS) -o bexample build_index_Example.c fm_index.a
	
run_queries:	run_queries.c fm_index.a
	$(CC) $(CFLAGS) run_queries.c fm_index.a -o FMindexv2
	
build_index:	build_index.c fm_index.a ds_ssortr
	$(CC) $(CFLAGS) -o FMindexv2b build_index.c fm_index.a

# archive containing fm-library
fm_index.a: fm_mng_bits.o fm_common.o fm_search.o fm_errors.o fm_read.o fm_occurences.o fm_multihuf.o fm_huffman.o fm_extract.o fm_build.o ds_ssort/ds.o ds_ssort/globals.o ds_ssort/helped.o ds_ssort/shallow.o ds_ssort/ds.o ds_ssort/globals.o ds_ssort/helped.o ds_ssort/shallow.o ds_ssort/deep2.o ds_ssort/blind2.o
	ar rc fm_index.a fm_common.o fm_mng_bits.o fm_search.o fm_build.o fm_errors.o fm_read.o fm_occurences.o fm_multihuf.o fm_huffman.o fm_extract.o ds_ssort/ds.o ds_ssort/globals.o ds_ssort/helped.o ds_ssort/shallow.o ds_ssort/ds.o ds_ssort/globals.o ds_ssort/helped.o ds_ssort/shallow.o ds_ssort/deep2.o ds_ssort/blind2.o
	
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
	rm -f *.o  fm_index.a *~ *.a ds_ssort/*.a ds_ssort/*.o
	
release:	
	make clean; rm -f fm_search fm_build fmi_qshell fmi_bshell bexample ds_ssort/ds ds_ssort/bwt ds_ssort/unbwt ds_ssort/testlcp;

ds_ssortr: 
	make -C ./ds_ssort/; cp ./ds_ssort/ds_ssort.a .
