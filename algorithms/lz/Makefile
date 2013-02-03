#FLAGS = -g -lm
FLAGS = -O9 -lm -fomit-frame-pointer -W -Wall -Winline -DDEBUG=0 -DNDEBUG=1
#FLAGS = -O9 -DQUERYREPORT -lm -fomit-frame-pointer -W -Wall -Winline -DDEBUG=0 -DNDEBUG=1
#FLAGS = -O9 -DQUERYTIME 
#FLAGS = -O9 -DINDEXREPORT 
#FLAGS = -O9 -DINDEXREPORT -DMEMCTRL

all: lzindex.a build_index run_queries

build_index: build_index.c lzindex.a
	gcc $(FLAGS) -o build_index build_index.c lzindex.a
run_queries: run_queries.c lzindex.a
	gcc $(FLAGS) -o run_queries run_queries.c lzindex.a

clean:
	rm -f *.o *.a


# generates the LZ-index library

lzindex.a: lzindex.o lztrie.o revtrie.o nodemap.o position.o trie.o heap.o parentheses.o bitmap.o hash.o mappings.o basics.o
	ar rc lzindex.a lzindex.o lztrie.o revtrie.o nodemap.o position.o trie.o heap.o parentheses.o bitmap.o hash.o mappings.o basics.o 

run_queries.o: run_queries.c
	gcc $(FLAGS) -c run_queries.c
build_index.o: build_index.c
	gcc $(FLAGS) -c build_index.c
lzindex.o: lzindex.c search.c index.c persist.c
	gcc $(FLAGS) -c lzindex.c
lztrie.o: lztrie.c
	gcc $(FLAGS) -c lztrie.c
revtrie.o: revtrie.c
	gcc $(FLAGS) -c revtrie.c
nodemap.o: nodemap.c
	gcc $(FLAGS) -c nodemap.c
position.o: position.c
	gcc $(FLAGS) -c position.c
mappings.o: mappings.c
	gcc $(FLAGS) -c mappings.c
trie.o: trie.c
	gcc $(FLAGS) -c trie.c
heap.o: heap.c
	gcc $(FLAGS) -c heap.c 
parentheses.o: parentheses.c
	gcc $(FLAGS) -c parentheses.c
bitmap.o: bitmap.c
	gcc $(FLAGS) -c bitmap.c
hash.o: hash.c
	gcc $(FLAGS) -c hash.c
basics.o: basics.c
	gcc $(FLAGS) -c basics.c 

basics.h: Makefile
	touch basics.h
bitmap.h: basics.h
	touch bitmap.h
mappings.h: bitmap.h revtrie.h lztrie.h
	touch mappings.h
hash.h: basics.h
	touch hash.h
lzindex.h: lztrie.h revtrie.h nodemap.h position.h mappings.h
	touch lzindex.h
lztrie.h: basics.h parentheses.h 
	touch lztrie.h
position.h: lztrie.h nodemap.h
	touch position.h
nodemap.h: basics.h lztrie.h
	touch nodemap.h
parentheses.h: basics.h bitmap.h hash.h
	touch parentheses.h
revtrie.h: basics.h parentheses.h lztrie.h nodemap.h
	touch revtrie.h
index.h: lzindex.h
	touch index.h
trie.h: basics.h heap.h
	touch trie.h
heap.h: basics.h
	touch heap.h
basics.c: basics.h
	touch basics.c
mappings.c: mappings.h
	touch mappings.c
hash.c: hash.h
	touch hash.c
bitmap.c: bitmap.h
	touch bitmap.c
lzindex.c: lzindex.h
	touch lzindex.c 
lztrie.c: lztrie.h
	touch lztrie.c
position.c: position.h
	touch position.c
nodemap.c: nodemap.h
	touch nodemap.c
parentheses.c: parentheses.h
	touch parentheses.c
revtrie.c: revtrie.h
	touch revtrie.c
trie.c: trie.h
	touch trie.c
heap.c: heap.h
	touch heap.c
run_queries.c: interface.h
	touch run_queries.c
build_index.c: interface.h
	touch build_index.c
