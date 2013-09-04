#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "sparsexml.h"

unsigned char priv_sxml_change_parser_state(SXMLParser* parser, enum SXMLParserState state) {
  static unsigned int index = 0;
  unsigned char ret;
  int i;

  if (strlen(parser->buffer) > 0) {
    if (parser->state == IN_TAG && state == IN_CONTENT && parser->tag_func != NULL) {
      ret = parser->tag_func(parser->buffer);
    } else if (parser->state == IN_TAG && state == IN_TAG && parser->tag_func != NULL) {
      ret = parser->tag_func(parser->buffer);
    } else if (parser->state == IN_TAG && state == IN_ATTRIBUTE_KEY && parser->tag_func != NULL) {
      ret = parser->tag_func(parser->buffer);
    } else if (parser->state == IN_CONTENT && state == IN_TAG && parser->content_func != NULL) {
      ret = parser->content_func(parser->buffer);
    } else if (parser->state == IN_ATTRIBUTE_KEY && state == IN_ATTRIBUTE_VALUE && parser->attribute_key_func != NULL) {
      ret = parser->attribute_key_func(parser->buffer);
    } else if (parser->state == IN_ATTRIBUTE_VALUE && state == IN_TAG && parser->attribute_value_func != NULL) {
      ret = parser->attribute_value_func(parser->buffer);
    }
  }

  parser->bp = 0;
  parser->buffer[0] = '\0';

  parser->state = state;

  return ret;

}

void sxml_init_parser(SXMLParser* parser) {
  int i;

  parser->state = INITIAL;
  parser->bp = 0;
  parser->buffer[0] = '\0';
}

void sxml_register_func(SXMLParser* parser, void* open, void* content, void* attribute_key, void* attribute_value) {
  parser->tag_func = open;
  parser->content_func = content;
  parser->attribute_value_func = attribute_value;
  parser->attribute_key_func = attribute_key;
}

unsigned char sxml_run_parser(SXMLParser* parser, char * xml) {

  unsigned char ret = SXMLParserContinue;

  do {

#ifdef __DEBUG1
    printf("State:%d Buffer:%s Char:%c\r\n", parser->state, parser->buffer, *xml);
#endif

    switch (parser->state) {
      case INITIAL:
        switch (*xml) {
          case '<':
            ret = priv_sxml_change_parser_state(parser, IN_TAG);
            continue;
        }
        break;
      case IN_TAG:
        switch (*xml) {
          case '>':
            ret =  priv_sxml_change_parser_state(parser, IN_CONTENT);
            continue;
          case ' ':
            ret = priv_sxml_change_parser_state(parser, IN_ATTRIBUTE_KEY);
            continue;
        }
        break;
      case IN_ATTRIBUTE_KEY:
        switch (*xml) {
          case '>':
            ret = priv_sxml_change_parser_state(parser, IN_CONTENT);
            continue;
          case '"':
            parser->bp--;
            parser->buffer[parser->bp] = '\0';
            ret = priv_sxml_change_parser_state(parser, IN_ATTRIBUTE_VALUE);
            continue;
        }
      case IN_ATTRIBUTE_VALUE:
        switch (*xml) {
          case '"':
            ret = priv_sxml_change_parser_state(parser, IN_TAG);
            continue;
        }
        break;
      case IN_CONTENT:
        switch (*xml) {
          case '<':
            ret = priv_sxml_change_parser_state(parser, IN_TAG);
            continue;
        }
        break;
    }
    parser->buffer[parser->bp++] = *xml;
    parser->buffer[parser->bp] = '\0';


  } while (*(++xml) != '\0' && ret == SXMLParserContinue);

  if (ret == SXMLParserStop) {
    return SXMLParserInterrupted;
  }

  return SXMLParserComplete;

}

// <?xml version="1.0" encoding="UTF-8"?><env:Envelope xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:ilon="http://wsdl.echelon.com/web_services_ns/ilon100/v4.0/message/" xmlns:env="http://schemas.xmlsoap.org/soap/envelope/"><env:Body><ilon:Read><iLonItem><Item><UCPTname>Net/LON/iLON App/Digital Output 1/nviClaValue_1</UCPTname></Item></iLonItem></ilon:Read></env:Body></env:Envelope>

/*
unsigned char storage[10][1024];
unsigned char onName=0x00;

void tag_open(char *tagname) {
  printf("tag: %s\r\n", tagname);
}

void tag_content(char *content) {
  printf("content: %s\r\n", content);
}

void key(char *name) {
  printf("attribute key: %s\n", name);
}

void value(char *name) {
  printf("attribute value: %s\n", name);
}

void main (void) {
  unsigned char xml[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><env:Envelope xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:ilon=\"http://wsdl.echelon.com/web_services_ns/ilon100/v4.0/message/\" xmlns:env=\"http://schemas.xmlsoap.org/soap/envelope/\"><env:Body><ilon:Read><iLonItem><Item><UCPTname>Net/LON/iLON App/Digital Output 1/nviClaValue_1</UCPTname></Item></iLonItem></ilon:Read></env:Body></env:Envelope>";

  SXMLParser parser;
  sxml_init_parser(&parser);
  sxml_register_func(&parser, &tag_open, &tag_content, &key, &value, (unsigned char*)storage);
  sxml_run_parser(&parser, xml);
}
*/
