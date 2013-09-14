#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "sparsexml-priv.h"

unsigned char priv_sxml_change_parser_state(SXMLParser* parser, enum SXMLParserState state) {
  unsigned char ret = SXMLParserContinue;

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

SXMLParser* sxml_init_parser(void) {
  SXMLParser* parser;
  parser = malloc(sizeof(SXMLParser));

  parser->state = INITIAL;
  parser->bp = 0;
  parser->buffer[0] = '\0';
  parser->header_parsed = 0;

  return parser;
}

void sxml_destroy_parser(SXMLParser *parser) {
  free(parser);
}

void sxml_register_func(SXMLParser* parser, void* open, void* content, void* attribute_key, void* attribute_value) {
  parser->tag_func = open;
  parser->content_func = content;
  parser->attribute_value_func = attribute_value;
  parser->attribute_key_func = attribute_key;
}

unsigned char sxml_run_parser(SXMLParser* parser, char *xml) {

  unsigned char result = SXMLParserContinue;

  do {

#ifdef  __DEBUG1
    printf("State:%d Buffer:%s CharAddr: %p Char:%c, result %d, length %d\r\n", parser->state, parser->buffer, xml, *xml, result, len);
#endif

    switch (parser->state) {
      case INITIAL:
        switch (*xml) {
          case '<':
            result = priv_sxml_change_parser_state(parser, INITIAL);
            continue;
          case '?':
            result = priv_sxml_change_parser_state(parser, IN_HEADER);
            continue;
        }
        break;
      case IN_HEADER:
        switch (*xml) {
          case '>':
            result = priv_sxml_change_parser_state(parser, IN_CONTENT);
            continue;
        }
        break;
      case IN_TAG:
        switch (*xml) {
          case '>':
            result =  priv_sxml_change_parser_state(parser, IN_CONTENT);
            continue;
          case ' ':
            result = priv_sxml_change_parser_state(parser, IN_ATTRIBUTE_KEY);
            continue;
        }
        break;
      case IN_ATTRIBUTE_KEY:
        switch (*xml) {
          case '>':
            result = priv_sxml_change_parser_state(parser, IN_CONTENT);
            continue;
          case '"':
            assert(parser->bp > 1);
            parser->bp--;
            parser->buffer[parser->bp] = '\0';
            result = priv_sxml_change_parser_state(parser, IN_ATTRIBUTE_VALUE);
            continue;
        }
      case IN_ATTRIBUTE_VALUE:
        switch (*xml) {
          case '"':
            result = priv_sxml_change_parser_state(parser, IN_TAG);
            continue;
        }
        break;
      case IN_CONTENT:
        switch (*xml) {
          case '<':
            result = priv_sxml_change_parser_state(parser, IN_TAG);
            continue;
        }
        break;
    }
    parser->buffer[parser->bp++] = *xml;
    parser->buffer[parser->bp] = '\0';

  } while ((*++xml != '\0') && (result == SXMLParserContinue));

  if (result == SXMLParserStop) {
    return SXMLParserInterrupted;
  }

  return SXMLParserComplete;

}

