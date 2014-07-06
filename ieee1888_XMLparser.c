
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ieee1888.h"
#include "ieee1888_XMLparser.h"

#define XML_ELEMENT_BEGIN   1
#define XML_ATTRIBUTE       2
#define XML_TEXT            3
#define XML_ELEMENT_END     4
#define XML_OTHERS	    5

#define MAX_XML_NODE_NS_LEN      30
#define MAX_XML_NODE_NAME_LEN    30
#define MAX_XML_NODE_VALUE_LEN 1024

struct _XML_Node {
int type;
char ns[MAX_XML_NODE_NS_LEN];
char name[MAX_XML_NODE_NAME_LEN];
char value[MAX_XML_NODE_VALUE_LEN];
};

typedef struct _XML_Node XMLNode;

const char* parse_elementXML(const char* strbuf, int len, XMLNode* node);
const char* parse_attributeXML(const char* strbuf, int len, XMLNode* node);
const char* parse_textXML(const char* strbuf, int len, XMLNode* node);
const char* parseXML(const char* strbuf, int len, XMLNode* node);

const char* ieee1888_parseXML_key(const char* strbuf, int len, ieee1888_key* key);
const char* ieee1888_parseXML_OK(const char* strbuf, int len, ieee1888_OK* ok);
const char* ieee1888_parseXML_error(const char* strbuf, int len, ieee1888_error* error);
const char* ieee1888_parseXML_query(const char* strbuf, int len, ieee1888_query* query);
const char* ieee1888_parseXML_header(const char* strbuf, int len, ieee1888_header* header);
const char* ieee1888_parseXML_value(const char* strbuf, int len, ieee1888_value* value);
const char* ieee1888_parseXML_point(const char* strbuf, int len, ieee1888_point* point);
const char* ieee1888_parseXML_pointSet(const char* strbuf, int len, ieee1888_pointSet* pointSet);
const char* ieee1888_parseXML_body(const char* strbuf, int len, ieee1888_body* body);
const char* ieee1888_parseXML_transport(const char* strbuf, int len, ieee1888_transport* transport);


const char* parse_elementXML(const char* strbuf, int len, XMLNode* node){

  int i;
  const char* p;
  char* q;
  p=strbuf;

  char buffer[(MAX_XML_NODE_NS_LEN > MAX_XML_NODE_NAME_LEN ? MAX_XML_NODE_NS_LEN : MAX_XML_NODE_NAME_LEN)+1];
  if(*p=='<'){
    p++; 
    if(--len<=0){ return NULL; }
  }else{
    return NULL;
  }

  if(strncmp(p,"!--",3)==0){
    node->type=XML_OTHERS;
    node->ns[0]=0;
    node->name[0]=0;
    node->value[0]=0;

    p+=2;
    len-=2;

    // eats until "-->"
    while(!(*(p-2)=='-' && *(p-1)=='-' && *p=='>')){
      p++; len--;
      if(len<=0){
        return NULL;
      }
    }
    return p;

  }else if(*p=='?'){
    node->type=XML_OTHERS;
    node->ns[0]=0;
    node->name[0]=0;
    node->value[0]=0;

    p++; len--;

    // eats until "?>"
    while(!(*(p-1)=='?' && *p=='>')){
      p++; len--;
      if(len<=0){
        return NULL;
      }
    }
    return p;


  }else if(*p=='/'){
    node->type=XML_ELEMENT_END;
    p++; len--;
    
    q=buffer;
    for(i=0;*p!=':' && *p!='>' && i<MAX_XML_NODE_NS_LEN && i<MAX_XML_NODE_NAME_LEN;i++){
      *(q++)=*(p++); len--;
      if(len<=0){
        return NULL;
      }
    }
    if(*p=='>'){
      *q=0;
      node->ns[0]=0;
      strncpy(node->name,buffer,MAX_XML_NODE_NAME_LEN);
      return p;
    }else if(*p==':'){
      *q=0;
      strncpy(node->ns,buffer,MAX_XML_NODE_NS_LEN);

      q=buffer; p++; len--;
      for(i=0;*p!='>' && i<MAX_XML_NODE_NAME_LEN;i++){
        *(q++)=*(p++); len--;
	if(len<=0){
          return NULL;
	}
      }
      *q=0;
      strncpy(node->name,buffer,MAX_XML_NODE_NAME_LEN);
      return p;

    }else{
      fprintf(stderr,"Fatal Error: exceeded the maximum length of assumed XML element.");
      return NULL;
    }


  }else{
    node->type=XML_ELEMENT_BEGIN;
    
    q=buffer;
    for(i=0;*p!=':' && *p!=' ' && *p!='>' && *p!='/' && i<MAX_XML_NODE_NS_LEN && i<MAX_XML_NODE_NAME_LEN;i++){
      *(q++)=*(p++); len--;
      if(len<=0){
        return NULL;
      }
    }
    if(*p==' ' || *p=='>' || *p=='/'){
      *q=0;
      node->ns[0]=0;
      strncpy(node->name,buffer,MAX_XML_NODE_NAME_LEN);
      return p;
    }else if(*p==':'){
      *q=0;
      strncpy(node->ns,buffer,MAX_XML_NODE_NS_LEN);

      q=buffer; p++; len--;
      for(i=0;*p!=' ' && *p!='>' && i<MAX_XML_NODE_NAME_LEN;i++){
        *(q++)=*(p++); len--;
	if(len<=0){
	  return NULL;
	}
      }
      *q=0;
      strncpy(node->name,buffer,MAX_XML_NODE_NAME_LEN);
      return p;

    }else{
      fprintf(stderr,"Fatal Error: exceeded the maximum length of assumed XML element.");
      return NULL;
    }
  }

  // unreachable code
  return NULL;
}

const char* parse_attributeXML(const char* strbuf, int len, XMLNode* node){
  
  int i;
  const char* p;
  char* q;
  p=strbuf;

  int max_len=MAX_XML_NODE_VALUE_LEN;
  char buffer[max_len+1];

  node->type=XML_ATTRIBUTE;
    
  q=buffer;
  for(i=0;*p!=':' && *p!='=' && i<max_len;i++){
    *(q++)=*(p++); len--;
    if(len<=0){
      return NULL;
    }
  }

  if(*p=='='){
    *q=0;
    node->ns[0]=0;
    strncpy(node->name,buffer,MAX_XML_NODE_NAME_LEN);

    // parse value
    p++; len--;
    if(*p=='\"'){ p++; len--; }
    q=buffer;
    for(i=0;*p!='\"' && *p!=' ' && i<MAX_XML_NODE_VALUE_LEN;i++){
      *(q++)=*(p++); len--;
      if(len<=0){
        return NULL;
      }
    }
    *q=0;
    strncpy(node->value,buffer,MAX_XML_NODE_VALUE_LEN);
    if(*p=='\"'){ p++; len--; }
    return p;

  }else if(*p==':'){
    *q=0;
    strncpy(node->ns,buffer,MAX_XML_NODE_NS_LEN);

    q=buffer; p++; len--;
    for(i=0;*p!='=' && i<MAX_XML_NODE_NAME_LEN;i++){
      *(q++)=*(p++); len--;
      if(len<=0){
        return NULL;
      }
    }
    *q=0;
    strncpy(node->name,buffer,MAX_XML_NODE_NAME_LEN);
    
    // parse value
    p++; len--;
    if(*p=='\"'){ p++; len--; }
    q=buffer;
    for(i=0;*p!='\"' && *p!=' ' && i<MAX_XML_NODE_VALUE_LEN;i++){
      *(q++)=*(p++); len--;
      if(len<=0){
        return NULL;
      }
    }
    *q=0;
    strncpy(node->value,buffer,MAX_XML_NODE_VALUE_LEN);
    if(*p=='\"'){ p++; len--; }
    return p;

  }else{
    fprintf(stderr,"Fatal Error: exceeded the maximum length of assumed XML element.");
    return NULL;
  }

  // unreachable code
  return NULL;
}


const char* parse_textXML(const char* strbuf, int len, XMLNode* node){
  
  int i;
  const char* p;
  char* q;
  p=strbuf;

  char buffer[MAX_XML_NODE_VALUE_LEN+1];
  if(*p=='>'){
    p++; len--;
    if(len<=0){
      return NULL;
    }
  }else{
    return NULL;
  }

  node->type=XML_TEXT;
  q=buffer;
  for(i=0;*p!='<' && i<MAX_XML_NODE_VALUE_LEN;i++){
    *(q++)=*(p++); len--;
    if(len<=0){
      return NULL;
    }
  }
  *q=0;
  node->ns[0]=0;
  strncpy(node->value,buffer,MAX_XML_NODE_VALUE_LEN);
  return p;
}

const char* parseXML(const char* strbuf, int len, XMLNode* node){

	const char* p=strbuf;

	// if NULL character return NULL
	if(*p=='\0'){
	  return NULL;
	}
	
	// eats blanks
	while(*p==' ' || *p=='\r' || *p=='\n'  ){
	  p++; len--;
	  if(len<=0){
            return NULL;
	  }
	}

	// ATTRIBUTE_NODE
	if((*p>='a' && *p<='z') || (*p>='A' && *p<='Z')){
	  return parse_attributeXML(p, len, node);
	}

	switch(*p){
	// ELEMENT_NODE (or COMMENTS) (might be BEGIN or END)
	case '<':
	  return parse_elementXML(p, len, node);

	// ELEMENT_END_NODE
	case '/':
	  node->type=XML_ELEMENT_END;
	  node->ns[0]=0;
	  node->name[0]=0;
	  node->value[0]=0;
          if(*(p+1)!='>'){
	    fprintf(stderr,"Fatal error in parsing XML.");
	    return NULL;
	  }
	  return p+1;

	// TEXT_NODE
	case '>':
	  return parse_textXML(p, len,node);

	// OTHERS
	default:
	  break;

	}


	node->type=XML_OTHERS;
	return p;
}


const char* ieee1888_parseXML_key(const char* strbuf, int len, ieee1888_key* key){

  const char* p;
  const char* q;
  p=strbuf;

  XMLNode node;
  while((q=parseXML(p, len,&node))!=NULL){
    switch(node.type){
    case XML_ELEMENT_BEGIN:
      if(strcmp(node.name,"key")!=0){
        // error
        fprintf(stderr,"Fatal error: unknown object \"%s\" appeared in \"key\"\n", node.name);
	return NULL;
      }
      break;

    case XML_ATTRIBUTE:
      if(strcmp(node.name,"id")==0){
        key->id=ieee1888_mk_string(node.value);

      }else if(strcmp(node.name,"attrName")==0){
        key->attrName=ieee1888_mk_attrNameType(node.value);

      }else if(strcmp(node.name,"select")==0){
        key->select=ieee1888_mk_selectType(node.value);

      }else if(strcmp(node.name,"lt")==0){
        key->lt=ieee1888_mk_string(node.value);

      }else if(strcmp(node.name,"gt")==0){
        key->gt=ieee1888_mk_string(node.value);

      }else if(strcmp(node.name,"lteq")==0){
        key->lteq=ieee1888_mk_string(node.value);

      }else if(strcmp(node.name,"gteq")==0){
        key->gteq=ieee1888_mk_string(node.value);

      }else if(strcmp(node.name,"eq")==0){
        key->eq=ieee1888_mk_string(node.value);

      }else if(strcmp(node.name,"neq")==0){
        key->neq=ieee1888_mk_string(node.value);

      }else if(strcmp(node.name,"trap")==0){
        key->trap=ieee1888_mk_trapType(node.value);

      }else{
        // error
        fprintf(stderr,"Fatal error: unknown attribute \"%s\" in key object \n",node.name);
	return NULL;
      }
      break;
    case XML_TEXT:
      break;

    case XML_ELEMENT_END:
      return q;
    
    default: 
      // error
      fprintf(stderr,"Fatal error: unknown error in processing key, maybe the XML is broken.\n");
      q++;
    }
    len-=(q-p);
    p=q;
  }

  // unreachable (if XML is well-formed)
  return NULL;
}

const char* ieee1888_parseXML_OK(const char* strbuf, int len, ieee1888_OK* ok){
  
  const char* p;
  const char* q;
  p=strbuf;

  XMLNode node;
  while((q=parseXML(p, len, &node))!=NULL){
    switch(node.type){
    case XML_ELEMENT_BEGIN:
      if(strcmp(node.name,"OK")!=0){
        // error
        fprintf(stderr,"Fatal error: unknown object \"%s\" appeared in \"OK\"\n", node.name);
	return NULL;
      }
      break;

    case XML_ATTRIBUTE:
      // error
      fprintf(stderr,"Fatal error: unknown attribute \"%s\" in OK object \n",node.name);
      return NULL;
      break;

    case XML_ELEMENT_END:
      return q;
    
    default: 
      // error
      fprintf(stderr,"Fatal error: unknown error in processing OK, maybe the XML is broken.\n");
      q++;
    }
    len-=(q-p);
    p=q;
  }

  // unreachable (if XML is well-formed)
  return NULL;
}

const char* ieee1888_parseXML_error(const char* strbuf, int len, ieee1888_error* error){
  
  const char* p;
  const char* q;
  p=strbuf;

  XMLNode node;
  while((q=parseXML(p,len,&node))!=NULL){
    switch(node.type){
    case XML_ELEMENT_BEGIN:
      if(strcmp(node.name,"error")!=0){
        // error
        fprintf(stderr,"Fatal error: unknown object \"%s\" appeared in \"error\"\n", node.name);
	return NULL;
      }
      break;

    case XML_ATTRIBUTE:
      if(strcmp(node.name,"type")==0){
        error->type=ieee1888_mk_string(node.value);

      }else{
        // error
        fprintf(stderr,"Fatal error: unknown attribute \"%s\" in error object \n",node.name);
	return NULL;
      }
      break;

    case XML_TEXT:
      error->content=ieee1888_mk_string(node.value);
      break;

    case XML_ELEMENT_END:
      return q;

    default: 
      // error
      fprintf(stderr,"Fatal error: unknown error in processing error, maybe the XML is broken.\n");
      q++;
    }
    len-=(q-p);
    p=q;
  }

  return NULL;
}

const char* ieee1888_parseXML_query(const char* strbuf, int len, ieee1888_query* query){
 
  const char* p;
  const char* q;
  p=strbuf;

  // management of childlen (key)
  ieee1888_key* p_key=NULL;
  ieee1888_key* q_key=NULL;
  int		n_key=0;

  XMLNode node;
  while((q=parseXML(p,len,&node))!=NULL){
    switch(node.type){
    case XML_ELEMENT_BEGIN:
      if(strcmp(node.name,"key")==0){

         // call ieee1888_parseXML_key(...);
	 ieee1888_key* add=ieee1888_mk_key();
	 if(add!=NULL){
           if((q=ieee1888_parseXML_key(p,len,add))==NULL){
	     ieee1888_destroy_objects((ieee1888_object*)add);
	     free(add);
	     return NULL;
	   }
	   q_key=ieee1888_add_key_to_array(p_key,n_key,add);
	   if(p_key!=NULL){ free(p_key); }
	   p_key=q_key; n_key++;
	   free(add);
	 }

      }else if(strcmp(node.name,"query")!=0){
        // error
        fprintf(stderr,"Fatal error: unknown object \"%s\" appeared in \"query\"\n", node.name);
	return NULL;
      }
      break;

    case XML_ATTRIBUTE:
      if(strcmp(node.name,"id")==0){
        query->id=ieee1888_mk_uuid(node.value);

      }else if(strcmp(node.name,"type")==0){
        query->type=ieee1888_mk_queryType(node.value);

      }else if(strcmp(node.name,"cursor")==0){
        query->cursor=ieee1888_mk_uuid(node.value);

      }else if(strcmp(node.name,"ttl")==0){
        query->ttl=atol(node.value);

      }else if(strcmp(node.name,"acceptableSize")==0){
        query->acceptableSize=atol(node.value);

      }else if(strcmp(node.name,"callbackData")==0){
        query->callbackData=ieee1888_mk_url(node.value);

      }else if(strcmp(node.name,"callbackControl")==0){
        query->callbackControl=ieee1888_mk_url(node.value);

      }else{
        // error
        fprintf(stderr,"Fatal error: unknown attribute \"%s\" in query object \n",node.name);
	return NULL;
      }
      break;

    case XML_TEXT:
      break;

    case XML_ELEMENT_END:
      // join parsed keys (children) to query object
      if(p_key!=NULL){
        query->key=p_key;
	query->n_key=n_key;
      }
      return q;

    default: 
      // error
      fprintf(stderr,"Fatal error: unknown error in processing query, maybe the XML is broken.\n");
      q++;
    }
    len-=(q-p);
    p=q;
  }

  return NULL;
}

const char* ieee1888_parseXML_header(const char* strbuf, int len, ieee1888_header* header){
  
  const char* p;
  const char* q;
  p=strbuf;

  XMLNode node;
  while((q=parseXML(p,len,&node))!=NULL){
    switch(node.type){
    case XML_ELEMENT_BEGIN:
      if(strcmp(node.name,"error")==0){
         // call ieee1888_parseXML_error(...);
	 ieee1888_error* error=ieee1888_mk_error();
	 if((q=ieee1888_parseXML_error(p,len,error))==NULL){
	   ieee1888_destroy_objects((ieee1888_object*)error);
           free(error);
	   return NULL;
	 }
	 header->error=error;
      }else if(strcmp(node.name,"OK")==0){
         // call ieee1888_parseXML_OK(...);
	 ieee1888_OK* ok=ieee1888_mk_OK();
	 if((q=ieee1888_parseXML_OK(p,len,ok))==NULL){
           free(ok);
	   return NULL;
	 }
	 header->OK=ok;
      }else if(strcmp(node.name,"query")==0){
         // call ieee1888_parseXML_query(...);
	 ieee1888_query* query=ieee1888_mk_query();
	 if((q=ieee1888_parseXML_query(p,len,query))==NULL){
	   ieee1888_destroy_objects((ieee1888_object*)query);
	   free(query);
           return NULL;
	 }
	 header->query=query;
      }else if(strcmp(node.name,"header")!=0){
        // error
        fprintf(stderr,"Fatal error: unknown object \"%s\" appeared in \"header\"\n", node.name);
	return NULL;
      }
      break;

    case XML_ATTRIBUTE:
      // error
      fprintf(stderr,"Fatal error: unknown attribute \"%s\" in header object \n",node.name);
      return NULL;
      break;

    case XML_TEXT:
      break;

    case XML_ELEMENT_END:
      return q;

    default: 
      // error
      fprintf(stderr,"Fatal error: unknown error in processing header, maybe the XML is broken.\n");
      q++;
    }
    len-=(q-p);
    p=q;
  }

  return NULL;
}

const char* ieee1888_parseXML_value(const char* strbuf, int len, ieee1888_value* value){
  
  const char* p;
  const char* q;
  p=strbuf;

  XMLNode node;
  while((q=parseXML(p,len,&node))!=NULL){
    switch(node.type){
    case XML_ELEMENT_BEGIN:
      if(strcmp(node.name,"value")!=0){
         // error
        fprintf(stderr,"Fatal error: unknown object \"%s\" appeared in \"value\"\n", node.name);
	return NULL;
      }
      break;

    case XML_ATTRIBUTE:
      if(strcmp(node.name,"time")==0){
        value->time=ieee1888_mk_time_from_string(node.value);

      }else{
        // error
        fprintf(stderr,"Fatal error: unknown attribute \"%s\" in value object \n",node.name);
	return NULL;
      }
      break;

    case XML_TEXT:
      value->content=ieee1888_mk_string(node.value);
      break;

    case XML_ELEMENT_END:
      return q;

    default: 
      // error
      fprintf(stderr,"Fatal error: unknown error in processing value, maybe the XML is broken.\n");
      q++;
    }
    len-=(q-p);
    p=q;
  }

  return NULL;
}

const char* ieee1888_parseXML_point(const char* strbuf, int len, ieee1888_point* point){
  
  const char* p;
  const char* q;
  p=strbuf;

  // management of childlen (value)
  ieee1888_value* p_value=NULL;
  ieee1888_value* q_value=NULL;
  int		n_value=0;

  XMLNode node;
  while((q=parseXML(p,len,&node))!=NULL){
    switch(node.type){
    case XML_ELEMENT_BEGIN:
      if(strcmp(node.name,"value")==0){

         // call ieee1888_parseXML_value(...);
	 ieee1888_value* add=ieee1888_mk_value();
	 if(add!=NULL){
           if((q=ieee1888_parseXML_value(p,len,add))==NULL){
	     ieee1888_destroy_objects((ieee1888_object*)add);
	     free(add);
	     return NULL;
	   }
	   q_value=ieee1888_add_value_to_array(p_value,n_value,add);
	   if(p_value!=NULL){ free(p_value); }
	   p_value=q_value; n_value++;
	   free(add);
	 }

      }else if(strcmp(node.name,"point")!=0){
        // error
        fprintf(stderr,"Fatal error: unknown object \"%s\" appeared in \"point\"\n", node.name);
	return NULL;
      }
      break;

    case XML_ATTRIBUTE:
      if(strcmp(node.name,"id")==0){
        point->id=ieee1888_mk_uuid(node.value);

      }else{
        // error
        fprintf(stderr,"Fatal error: unknown attribute \"%s\" in point object \n",node.name);
	return NULL;
      }
      break;

    case XML_TEXT:
      break;

    case XML_ELEMENT_END:
      // join parsed values (children) to point object
      if(p_value!=NULL){
        point->value=p_value;
	point->n_value=n_value;
      }
      return q;

    default: 
      // error
      fprintf(stderr,"Fatal error: unknown error in processing point, maybe the XML is broken.\n");
      q++;
    }
    len-=(q-p);
    p=q;
  }
  return NULL;
}

const char* ieee1888_parseXML_pointSet(const char* strbuf, int len, ieee1888_pointSet* pointSet){

  const char* p;
  const char* q;
  p=strbuf;

  // management of recursive pointSet definition
  int is_first_pointSet=1;

  // management of childlen (pointSet & point)
  ieee1888_pointSet* p_pointSet=NULL;
  ieee1888_pointSet* q_pointSet=NULL;
  int		n_pointSet=0;
  ieee1888_point* p_point=NULL;
  ieee1888_point* q_point=NULL;
  int		n_point=0;

  XMLNode node;
  while((q=parseXML(p,len,&node))!=NULL){

    switch(node.type){
    case XML_ELEMENT_BEGIN:

      if(strcmp(node.name,"pointSet")==0){

        if(is_first_pointSet){
          is_first_pointSet=0;
	}else{
          // call ieee1888_parseXML_pointSet(...);
	  ieee1888_pointSet* add=ieee1888_mk_pointSet();
	  if(add!=NULL){
            if((q=ieee1888_parseXML_pointSet(p,len,add))==NULL){
	      ieee1888_destroy_objects((ieee1888_object*)add);
	      free(add);
	      return NULL;
	    }
	    q_pointSet=ieee1888_add_pointSet_to_array(p_pointSet,n_pointSet,add);
	    if(p_pointSet!=NULL){ free(p_pointSet); }
	    p_pointSet=q_pointSet; n_pointSet++;
	    free(add);
	  }
        }

      }else if(strcmp(node.name,"point")==0){
          
	  // call ieee1888_parseXML_point(...);
	  ieee1888_point* add=ieee1888_mk_point();
	  if(add!=NULL){
            if((q=ieee1888_parseXML_point(p,len,add))==NULL){
              ieee1888_destroy_objects((ieee1888_object*)add);
	      free(add);
	      return NULL;
	    }
	    q_point=ieee1888_add_point_to_array(p_point,n_point,add);
	    if(p_point!=NULL){ free(p_point); }
	    p_point=q_point; n_point++;
	    free(add);
	  }

      }else{
        // error
        fprintf(stderr,"Fatal error: unknown object \"%s\" appeared in \"pointSet\"\n", node.name);
	return NULL;
      }
      break;

    case XML_ATTRIBUTE:
      if(strcmp(node.name,"id")==0){
        pointSet->id=ieee1888_mk_uuid(node.value);

      }else{
        // error
        fprintf(stderr,"Fatal error: unknown attribute \"%s\" in point object \n",node.name);
	return NULL;
      }
      break;

    case XML_TEXT:
      break;

    case XML_ELEMENT_END:
      // join parsed pointSet (children) to pointSet object
      if(p_pointSet!=NULL){
        pointSet->pointSet=p_pointSet;
	pointSet->n_pointSet=n_pointSet;
      }
      // join parsed point (children) to pointSet object
      if(p_point!=NULL){
        pointSet->point=p_point;
	pointSet->n_point=n_point;
      }
      return q;

    default: 
      // error
      fprintf(stderr,"Fatal error: unknown error, maybe the XML is broken.");
      q++;
    }
    len-=(q-p);
    p=q;
  }
  return NULL;
}

const char* ieee1888_parseXML_body(const char* strbuf, int len, ieee1888_body* body){
  
  const char* p;
  const char* q;
  p=strbuf;

  // management of childlen (pointSet & point)
  ieee1888_pointSet* p_pointSet=NULL;
  ieee1888_pointSet* q_pointSet=NULL;
  int		n_pointSet=0;
  ieee1888_point* p_point=NULL;
  ieee1888_point* q_point=NULL;
  int		n_point=0;

  XMLNode node;
  while((q=parseXML(p,len,&node))!=NULL){

    switch(node.type){
    case XML_ELEMENT_BEGIN:

      if(strcmp(node.name,"pointSet")==0){
          
	// call ieee1888_parseXML_pointSet(...);
	ieee1888_pointSet* add=ieee1888_mk_pointSet();
	if(add!=NULL){
          if((q=ieee1888_parseXML_pointSet(p,len,add))==NULL){
	    ieee1888_destroy_objects((ieee1888_object*)add);
	    free(add);
	    return NULL;
	  }
	  q_pointSet=ieee1888_add_pointSet_to_array(p_pointSet,n_pointSet,add);
	  if(p_pointSet!=NULL){ free(p_pointSet); }
	  p_pointSet=q_pointSet; n_pointSet++;
	  free(add);
	}
      
      }else if(strcmp(node.name,"point")==0){
          
        // call ieee1888_parseXML_point(...);
        ieee1888_point* add=ieee1888_mk_point();
	if(add!=NULL){
          if((q=ieee1888_parseXML_point(p,len,add))==NULL){
            ieee1888_destroy_objects((ieee1888_object*)add);
            free(add);
	    return NULL;
	  }
	  q_point=ieee1888_add_point_to_array(p_point,n_point,add);
	  if(p_point!=NULL){ free(p_point); }
	  p_point=q_point; n_point++;
	  free(add);
	}

      }else if(strcmp(node.name,"body")!=0){
        // error
        fprintf(stderr,"Fatal error: unknown object \"%s\" appeared in \"pointSet\"\n", node.name);
	return NULL;
      }
      break;
      
    case XML_ATTRIBUTE:
      // error
      fprintf(stderr,"Fatal error: unknown attribute \"%s\" in point object \n",node.name);
      return NULL;
      break;

    case XML_TEXT:
      break;

    case XML_ELEMENT_END:
      // join parsed pointSet (children) to body object
      if(p_pointSet!=NULL){
        body->pointSet=p_pointSet;
	body->n_pointSet=n_pointSet;
      }
      // join parsed point (children) to body object
      if(p_point!=NULL){
        body->point=p_point;
	body->n_point=n_point;
      }

      return q;
    
    default: 
      // error
      fprintf(stderr,"Fatal error: unknown error, maybe the XML is broken.");
      q++;
    }
    len-=(q-p);
    p=q;
  }
  return NULL;
}

const char* ieee1888_parseXML_transport(const char* strbuf, int len, ieee1888_transport* transport){
  
  const char* p;
  const char* q;
  p=strbuf;

  XMLNode node;
  while((q=parseXML(p,len,&node))!=NULL){
    switch(node.type){
    case XML_ELEMENT_BEGIN:

      if(strcmp(node.name,"header")==0){
         // call ieee1888_parseXML_header(...);
	 ieee1888_header* header=ieee1888_mk_header();
	 if((q=ieee1888_parseXML_header(p,len,header))==NULL){
           ieee1888_destroy_objects((ieee1888_object*)header);
	   free(header);
	   return NULL;
	 }
	 transport->header=header;

      }else if(strcmp(node.name,"body")==0){
         // call ieee1888_parseXML_body(...);
	 ieee1888_body* body=ieee1888_mk_body();
	 if((q=ieee1888_parseXML_body(p,len,body))==NULL){
	   ieee1888_destroy_objects((ieee1888_object*)body);
	   free(body);
	   return NULL;
	 }
	 transport->body=body;
      }else if(strcmp(node.name,"transport")!=0){
        // error
        fprintf(stderr,"Fatal error: unknown object \"%s\" appeared in \"transport\"\n", node.name);
	return NULL;
      }
      break;

    case XML_ATTRIBUTE:
      // error if node.name!="xmlns"
      if(strcmp(node.name,"xmlns")!=0){
        fprintf(stderr,"Fatal error: unknown attribute \"%s\" in transport object \n",node.name);
	return NULL;
      }
      break;

    case XML_TEXT:
      break;

    case XML_ELEMENT_END:
      return q;
    
    }
    len-=(q-p);
    p=q;
  }

  return NULL;
}

const char* ieee1888_soap_parse(const char* str_soap, int len, ieee1888_transport* transport, int* message){

  const char* p;
  const char* q;
  p=str_soap;

  XMLNode node;
  while((q=parseXML(p, len, &node))!=NULL){
    switch(node.type){
    case XML_ELEMENT_BEGIN:

      if(strcmp(node.name,"Envelope")==0){

      }else if(strcmp(node.name,"Header")==0){

      }else if(strcmp(node.name,"Body")==0){
      
      }else if(strcmp(node.name,"queryRQ")==0){
        *message=IEEE1888_QUERY_RQ;

      }else if(strcmp(node.name,"queryRS")==0){
        *message=IEEE1888_QUERY_RS;

      }else if(strcmp(node.name,"dataRQ")==0){
        *message=IEEE1888_DATA_RQ;

      }else if(strcmp(node.name,"dataRS")==0){
        *message=IEEE1888_DATA_RS;

      }else if(strcmp(node.name,"transport")==0){
	 if((q=ieee1888_parseXML_transport(p,len,transport))==NULL){
	   return NULL;
	 }

      }else{
      
      }
      break;

    case XML_ATTRIBUTE:
      break;

    case XML_TEXT:
      break;

    case XML_ELEMENT_END:
      if(strcmp(node.name,"Envelope")==0){
        return q;
      }
      break;

    default:
      // error
      // fprintf(stderr,"Fatal error: unknown error, maybe the XML is broken.");
      q++;

    }
    len-=(q-p);
    p=q;
  }
  return NULL;
}

/*

int main(int argc, char** argv){

   char line[1000];

   int n_buffer=0;
   char buffer[100000];

   FILE* fp=fopen("sample2.xml","r");

   while(fgets(line,1000,fp)!=NULL){
     int len=strlen(line);
     strcpy(&buffer[n_buffer],line);
     n_buffer+=len;
   }
   fclose(fp);

   n_buffer--;

   XMLNode node;
   const char* p=buffer;
   const char* q;
   int len=n_buffer;
   while((q=parseXML(p,len,&node))!=NULL){

     switch(node.type){
     case XML_ELEMENT_BEGIN:
       printf("XML_ELEMENT_BEGIN (ns,name)=(%s, %s)\n", node.ns, node.name);
       break;

     case XML_ELEMENT_END:
       printf("XML_ELEMENT_END (ns,name)=(%s, %s)\n", node.ns, node.name);
       break;

     case XML_ATTRIBUTE:
       printf("XML_ELEMENT_ATTRIBUTE (ns,name,value)=(%s, %s, %s)\n", node.ns, node.name, node.value);
       break;

     case XML_TEXT:
       printf("XML_TEXT (value)=(%s)\n", node.value);
       break;
    
     default: 
      printf("G"); fflush(stdout);
       // error
       fprintf(stderr,"Fatal error: unknown error, maybe the XML is broken.");
       q++;

     }
     p=q;
     len-=q-p;
   }

   p=buffer;
   ieee1888_transport* transport=ieee1888_mk_transport();
   q=ieee1888_parseXML_transport(p,n_buffer,transport);
   ieee1888_dump_objects((ieee1888_object*)transport);

   //char mybuf[10000];
   //if(ieee1888_soap_gen(transport,4,mybuf,10000)){
   //  printf(mybuf);
   //}
  
   return 0;
}


int main(int argc, char** argv){

   char line[1000];

   int n_buffer=0;
   char buffer[100000];

   FILE* fp=fopen("dataRS.soap","r");

   while(fgets(line,1000,fp)!=NULL){
     int len=strlen(line);
     strcpy(&buffer[n_buffer],line);
     n_buffer+=len;
   }
   fclose(fp);

   const char* p;
   const char* q;
   XMLNode node;
   p=buffer;
   len=n_buffer;
   while((q=parseXML(p,len,&node))!=NULL){

     switch(node.type){
     case XML_ELEMENT_BEGIN:
       printf("XML_ELEMENT_BEGIN (ns,name)=(%s, %s)\n", node.ns, node.name);
       break;

     case XML_ELEMENT_END:
       printf("XML_ELEMENT_END (ns,name)=(%s, %s)\n", node.ns, node.name);
       break;

     case XML_ATTRIBUTE:
       printf("XML_ELEMENT_ATTRIBUTE (ns,name,value)=(%s, %s, %s)\n", node.ns, node.name, node.value);
       break;

     case XML_TEXT:
       printf("XML_TEXT (value)=(%s)\n", node.value);
       break;
    
     default: 
      printf("H"); fflush(stdout);
       // error
       fprintf(stderr,"Fatal error: unknown error, maybe the XML is broken.");
       q++;
     }
     p=q;
     len-=q-p;
   }


   int message;
   p=buffer;
   ieee1888_transport* transport=ieee1888_mk_transport();
   q=ieee1888_soap_parse(p,transport,&message);
   ieee1888_dump_objects((ieee1888_object*)transport);

   printf("\n");
   printf("%d",message);
   printf("\n");

}

*/
