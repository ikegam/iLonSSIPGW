#include <CUnit/CUnit.h>

#include "sparsexml.h"

void test_initialize_parser(void) {
  SXMLParser parser;

  sxml_init_parser(&parser);
  CU_ASSERT (parser.state == INITIAL)
  CU_ASSERT (parser.bp == 0)
  CU_ASSERT (strlen(parser.buffer) == 0)
}

void test_parse_simple_xml(void) {
  SXMLParser parser;
  char xml[] = "<?xml version=\"1.1\"?><tag></tag>";
  unsigned char ret;

  unsigned char on_tag(char *name) {
    static int c = 0;
    if (strcmp("tag", name) == 0 && c == 0) {
      c++;
    } else if (strcmp("/tag", name) == 0 && c == 1) {
      c++;
    }
    if (c==2) {
      return SXMLParserStop;
    }
    return SXMLParserContinue;
  }

  sxml_init_parser(&parser);
  sxml_register_func(&parser, &on_tag, NULL, NULL, NULL);
  ret = sxml_run_parser(&parser, xml);
  CU_ASSERT (ret == SXMLParserInterrupted);
}

void test_parse_separated_xml(void) {
  SXMLParser parser;
  char xml1[] = "<?xml versi";
  char xml2[] = "on=\"1.1\"?";
  char xml3[] = "><ta";
  char xml4[] = "g></tag>";
  unsigned char ret;

  unsigned char on_tag(char *name) {
    static int c = 0;
    if (strcmp("tag", name) == 0 && c == 0) {
      c++;
    } else if (strcmp("/tag", name) == 0 && c == 1) {
      c++;
    }
    if (c==2) {
      return SXMLParserStop;
    }
    return SXMLParserContinue;
  }

  sxml_init_parser(&parser);
  sxml_register_func(&parser, &on_tag, NULL, NULL, NULL);
  ret = sxml_run_parser(&parser, xml1);
  CU_ASSERT (parser.state == IN_ATTRIBUTE_KEY);
  ret = sxml_run_parser(&parser, xml2);
  CU_ASSERT (parser.state == IN_TAG);
  ret = sxml_run_parser(&parser, xml3);
  CU_ASSERT (parser.state == IN_TAG);
  CU_ASSERT (strcmp(parser.buffer, "ta") == 0);
  ret = sxml_run_parser(&parser, xml4);
  CU_ASSERT (parser.state == IN_CONTENT);
  CU_ASSERT (ret == SXMLParserInterrupted);
}

void test_check_event_on_content(void) {
  SXMLParser parser;
  char xml[] = "<?xml version=\"1.1\"?><tag>1 0<tag2>2</tag2>3</tag>";
  unsigned char ret;

  unsigned char on_content(char *content) {
    static int c = 0;
    switch (content[0]) {
      case '1':
        if (content[2] == '0') {
          c++;
        }
        break;
      case '2':
        c++;
        break;
      case '3':
        c++;
        break;
    }
    if (c==2) {
      return SXMLParserStop;
    }
    return SXMLParserContinue;
  }

  sxml_init_parser(&parser);
  sxml_register_func(&parser, NULL, &on_content, NULL, NULL);
  ret = sxml_run_parser(&parser, xml);
  CU_ASSERT (ret == SXMLParserInterrupted);
}

int main(void) {
  CU_pSuite suite;
  CU_initialize_registry();

  suite = CU_add_suite("SparseXML", NULL, NULL);
  CU_add_test(suite, "initialize phase", test_initialize_parser);
  CU_add_test(suite, "Parse simple XML", test_parse_simple_xml);
  CU_add_test(suite, "Parse simple separated XML", test_parse_separated_xml);
  CU_add_test(suite, "Check status in running parser", test_check_event_on_content);
  CU_basic_run_tests();
  CU_cleanup_registry();

  return 0;
}
