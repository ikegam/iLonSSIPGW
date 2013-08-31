#ifndef ___IEEE1888_XMLPARSER_H___20111130___
#define ___IEEE1888_XMLPARSER_H___20111130___

#define IEEE1888_QUERY_RQ 1
#define IEEE1888_QUERY_RS 2
#define IEEE1888_DATA_RQ  3
#define IEEE1888_DATA_RS  4
const char* ieee1888_soap_parse(const char* str_soap, int len, ieee1888_transport* transport, int* message);

#endif /* #ifndef ___IEEE1888_XMLPARSER_H___20111130___ */
