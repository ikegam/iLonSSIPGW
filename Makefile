CC=gcc
COMMON_OBJS   = ieee1888_XMLgenerator.o ieee1888_XMLparser.o ieee1888_client.o ieee1888_object_factory.o ieee1888_server.o ieee1888_util.o ieee1888_datapool.o ilonss.o sparsexml/sparsexml.o
LFLAG = -lpthread
CFLAGS = -Wall -g -O0 -I./sparsexml

all:	ieee1888_ilonss_gw

ilonss.o: ilonss.c
	(cd sparsexml; make)
	$(CC) -c -Wall -g -O0 $(CFLAGS) $< sparsexml/sparsexml.c

.c.o:
	$(CC) -c -Wall -g -O0 $(CFLAGS) $<

ieee1888_ilonss_gw: ieee1888_ilonss_gw.o $(COMMON_OBJS)
		$(CC) $(COMMON_OBJS) ieee1888_ilonss_gw.o -o ieee1888_ilonss_gw $(LFLAG)

clean: 
	(cd sparsexml; make clean)
	rm -f *.o ieee1888_ilonss_gw.o ieee1888_ilonss_gw
