#ifndef __SXMLParser__
#define __SXMLParser__

enum SXMLParserState {
  INITIAL,
  IN_HEADER,
  IN_TAG,
  IN_ATTRIBUTE_KEY,
  IN_ATTRIBUTE_VALUE,
  IN_CONTENT
};

#define SXMLParserContinue 0x00
#define SXMLParserStop 0x01

#define SXMLParserComplete 0x02
#define SXMLParserInterrupted 0x03

#define SXMLElementLength 1024

typedef struct __SXMLParser {
  enum SXMLParserState state;

  char buffer[SXMLElementLength];
  unsigned int bp;
  unsigned char header_parsed;

  unsigned char (*tag_func)(char *);
  unsigned char (*content_func)(char *);
  unsigned char (*attribute_value_func)(char *);
  unsigned char (*attribute_key_func)(char *);
} SXMLParser;

unsigned char priv_sxml_change_parser_state(SXMLParser* parser, enum SXMLParserState state);
void sxml_init_parser(SXMLParser* parser);
void sxml_register_func(SXMLParser* parser, void* tag, void* content, void* attribute_key, void* attribute_value);
unsigned char sxml_run_parser(SXMLParser* parser, char * xml, int count);

#endif
