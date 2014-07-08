CC=gcc
LFLAG = -lpthread $(shell pkg-config --libs light1888)
CFLAGS = -Wall -g -O0 -I./sparsexml $(shell pkg-config --cflags light1888)

all:	ieee1888_ilonss_gw

sparsexml:
	git clone https://github.com/ikegam/sparsexml.git

ilonss.o: ilonss.c sparsexml
	(cd sparsexml; make)
	$(CC) -c -Wall -g -O0 $(CFLAGS) $< sparsexml/sparsexml.c

.c.o:
	$(CC) -c -Wall -g -O0 $(CFLAGS) $<

ieee1888_ilonss_gw: ieee1888_ilonss_gw.o ilonss.o sparsexml/sparsexml.o
		$(CC) $^ -o ieee1888_ilonss_gw $(LFLAG)

clean: 
	-(cd sparsexml; make clean)
	-rm -f *.o ieee1888_ilonss_gw.o ieee1888_ilonss_gw

.PHONY:clean all
