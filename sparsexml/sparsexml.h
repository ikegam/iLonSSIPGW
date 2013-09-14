#ifndef __SXMLParser__
#define __SXMLParser__

#define SXMLParserContinue 0x00
#define SXMLParserStop 0x01

#define SXMLParserComplete 0x02
#define SXMLParserInterrupted 0x03

#define SXMLElementLength 1024

typedef struct __SXMLParser SXMLParser;

SXMLParser* sxml_init_parser(void);
void sxml_destroy_parser(SXMLParser*);
void sxml_register_func(SXMLParser*, void*, void*, void*, void*);

unsigned char sxml_run_parser(SXMLParser*, char*);

#endif
