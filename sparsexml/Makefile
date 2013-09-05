all: test-sparsexml

test: test-sparsexml
	./$<

sparsexml.o: sparsexml.c
	$(CC) -Wall -g -O0 -c $<

test-sparsexml: sparsexml.o test.c
	$(CC) -Wall -g -O0 -o $@ $^ -lcunit

clean: sparsexml.o test-sparsexml
	rm $^

.PHONY: clean all test
