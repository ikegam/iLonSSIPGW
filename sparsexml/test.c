#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "sparsexml.h"

void add_private_test(CU_pSuite*);

void test_parse_simple_xml(void) {
  SXMLParser* parser;
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

  parser = sxml_init_parser();
  sxml_register_func(parser, &on_tag, NULL, NULL, NULL);
  ret = sxml_run_parser(parser, xml);
  CU_ASSERT (ret == SXMLParserInterrupted);
  sxml_destroy_parser(parser);
}

void test_check_event_on_content(void) {
  SXMLParser* parser;
  char xml[] = "<?xml version=\"1.1\"?><tag>1 0<tag2>2</tag2>3</tag>";
  unsigned char ret;

  unsigned char on_content(char *content) {
    static int c = 0;
    switch (content[0]) {
      case '1':
        if (content[2] == '0') {
          CU_ASSERT (c++ == 0);
        }
        break;
      case '2':
        c++;
        break;
      case '3':
        c++;
        break;
    }
    if (c==3) {
      return SXMLParserStop;
    }
    return SXMLParserContinue;
  }

  parser = sxml_init_parser();
  sxml_register_func(parser, NULL, &on_content, NULL, NULL);
  ret = sxml_run_parser(parser, xml);
  CU_ASSERT (ret == SXMLParserInterrupted);
  sxml_destroy_parser(parser);
}

void test_check_parsing_attribute(void) {
  SXMLParser* parser;
  char xml[] = "<?xml version=\"1.1\"?><tag hoge=\"fuga\" no=\"</tag>\"><tag2 goe=\"ds\"   /></tag>";
  unsigned int c=0;

  unsigned char on_attribute_key(char *name) {
    if (strcmp("hoge", name) == 0 && c == 1) {
      c++;
    }
    if (strcmp("no", name) == 0 && c == 3) {
      c++;
    }
    if (strcmp("goe", name) == 0 && c == 6) {
      c++;
    }
    return SXMLParserContinue;
  }

  unsigned char on_attribute_value(char *name) {
    if (strcmp("fuga", name) == 0 && c == 2) {
      c++;
    }
    if (strcmp("</tag>", name) == 0 && c == 4) {
      c++;
    }
    if (strcmp("ds", name) == 0 && c == 7) {
      c++;
    }
    return SXMLParserContinue;
  }

  unsigned char on_tag(char *name) {
    if (strcmp("tag", name) == 0 && c == 0) {
      c++;
    }
    if (strcmp("tag2", name) == 0 && c == 5) {
      c++;
    }
    if (strcmp("/tag", name) == 0 && c == 8) {
      c++;
    }
    return SXMLParserContinue;
  }

  parser = sxml_init_parser();
  sxml_register_func(parser, on_tag, NULL, on_attribute_key, on_attribute_value);
  sxml_run_parser(parser, xml);
  CU_ASSERT(c == 9);

  sxml_destroy_parser(parser);
}

int main(void) {
  CU_pSuite suite;
  CU_initialize_registry();

  suite = CU_add_suite("SparseXML", NULL, NULL);
  add_private_test(&suite);
  CU_add_test(suite, "Parse simple XML", test_parse_simple_xml);
  CU_add_test(suite, "Check status in running parser", test_check_event_on_content);
  CU_add_test(suite, "Check ", test_check_parsing_attribute );
  CU_basic_run_tests();
  CU_cleanup_registry();

  return 0;
}
