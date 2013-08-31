CC=gcc
COMMON_OBJS   = ieee1888_XMLgenerator.o ieee1888_XMLparser.o ieee1888_client.o ieee1888_object_factory.o ieee1888_server.o ieee1888_util.o ieee1888_datapool.o bacnetip.o
LFLAG = -lpthread

all:	ieee1888_bacnetip_gw

ieee1888_bacnetip_gw: ieee1888_bacnetip_gw.o $(COMMON_OBJS)
		$(CC) $(COMMON_OBJS) ieee1888_bacnetip_gw.o -o ieee1888_bacnetip_gw $(LFLAG)

clean: 
	rm -f *.o ieee1888_bacnetip_gw *~
