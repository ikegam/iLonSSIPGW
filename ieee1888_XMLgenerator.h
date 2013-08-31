#ifndef ___IEEE1888_XMLGENERATOR_H___20111130___
#define ___IEEE1888_XMLGENERATOR_H___20111130___

#define IEEE1888_QUERY_RQ 1
#define IEEE1888_QUERY_RS 2
#define IEEE1888_DATA_RQ  3
#define IEEE1888_DATA_RS  4

#define IEEE1888_SOAP_GEN_ERROR_UNKNOWN_MSG -1
#define IEEE1888_SOAP_GEN_ERROR_OVERFLOW    -2
int ieee1888_soap_gen(const ieee1888_transport* transport, int message, char* str_soap, int n);
int ieee1888_soap_error_gen(const char* error_msg,char* str_soap,int n);

#endif /* #ifndef ___IEEE1888_XMLGENERATOR_H___20111130___ */
