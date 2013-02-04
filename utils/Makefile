CFLAGS=-O9

all: cut compare alph entrop entrop2 entrop3 gentext genpatterns genintervals build_index run_queries 

cut: cut.c
	$(CC) $(CFLAGS) -o cut cut.c

compare: compare.c
	$(CC) $(CFLAGS) -o compare compare.c

alph: alph.c
	$(CC) $(CFLAGS) -o alph alph.c

entrop: entrop.c
	$(CC) $(CFLAGS) -o entrop entrop.c -lm

entrop2: entrop2.c
	$(CC) $(CFLAGS) -o entrop2 entrop2.c -lm

entrop3: entrop3.c
	$(CC) $(CFLAGS) -o entrop3 entrop3.c -lm

gentext: gentext.c
	$(CC) $(CFLAGS) -o gentext gentext.c

genpatterns: genpatterns.c
	$(CC) $(CFLAGS) -o genpatterns genpatterns.c

genintervals: genintervals.c
	$(CC) $(CFLAGS) -o genintervals genintervals.c

build_index: build_index.c ../algorithms/$(TARGET)/$(TARGET).a
	$(CC) $(CFLAGS) -o build_index build_index.c ../algorithms/$(TARGET)/$(TARGET).a

run_queries: run_queries.c ../algorithms/$(TARGET)/$(TARGET).a
	$(CC) $(CFLAGS) -o run_queries run_queries.c ../algorithms/$(TARGET)/$(TARGET).a

../algorithms/$(TARGET)/$(TARGET).a:
	$(MAKE) -C ../algorithms/$(TARGET) $(TARGET).a

clean:
	rm -f cut compare alph entrop entrop2 entrop3 gentext genpatterns genintervals run_queries build_index
