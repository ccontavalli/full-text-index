
all: cut compare alph entrop entrop2 entrop3 gentext genpatterns genintervals build_index run_queries 

cut: cut.c
	gcc -O9 -o cut cut.c

compare: compare.c
	gcc -O9 -o compare compare.c

alph: alph.c
	gcc -O9 -o alph alph.c

entrop: entrop.c
	gcc -O9 -o entrop entrop.c -lm

entrop2: entrop2.c
	gcc -O9 -o entrop2 entrop2.c -lm

entrop3: entrop3.c
	gcc -O9 -o entrop3 entrop3.c -lm

gentext: gentext.c
	gcc -O9 -o gentext gentext.c

genpatterns: genpatterns.c
	gcc -O9 -o genpatterns genpatterns.c

genintervals: genintervals.c
	gcc -O9 -o genintervals genintervals.c

#build_index.o: build_index.c
#	gcc -O9 build_index.c

build_index: 
	echo 'To make build_index: <compiler> -O9 -o build_index build_index.o <your index library>' ; echo 'The correct <compiler> depends on which one is used to build the library, gcc or g++'

#run_queries.o: run_queries.c
#	gcc -O9 run_queries.c

run_queries: 
	echo 'To make run_queries: <compiler> -O9 -o run_queries run_queries.o <your index library>' ; echo 'The correct <compiler> depends on which one is used to build the library, gcc or g++'

clean:
	rm -f cut compare alph entrop entrop2 entrop3 gentext genpatterns genintervals
