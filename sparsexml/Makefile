all: test-sparsexml

sparsexml.o: sparsexml.c
	$(CC) -g -O0 -c $<

test-sparsexml: sparsexml.o test.c
	$(CC) -g -o $@ $^ -lcunit

clean: sparsexml.o test-sparsexml
	rm $<

.PHONY: clean all
