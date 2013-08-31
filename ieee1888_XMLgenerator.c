
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ieee1888.h"
#include "ieee1888_XMLgenerator.h"

int ieee1888_generateXML(ieee1888_object* obj, char* strbuf, int n){

  if(obj==NULL){
    return 0;
  }

  int i,j;
  int offset=0;

  switch(obj->dtype){
 
  case IEEE1888_DATATYPE_KEY:
    {
      ieee1888_key* key=(ieee1888_key*)obj;
      i=sprintf(strbuf+offset,"<key");	offset+=i;
      if(key->id!=NULL){           i=sprintf(strbuf+offset," id=\"%s\"",key->id);              	offset+=i;      }
      if(key->attrName!=NULL){     i=sprintf(strbuf+offset," attrName=\"%s\"",key->attrName);   offset+=i;      }
      if(key->eq!=NULL){           i=sprintf(strbuf+offset," eq=\"%s\"",key->eq); 		offset+=i;  	} 
      if(key->neq!=NULL){          i=sprintf(strbuf+offset," neq=\"%s\"",key->neq); 		offset+=i;  	} 
      if(key->lt!=NULL){           i=sprintf(strbuf+offset," lt=\"%s\"",key->lt); 		offset+=i;  	} 
      if(key->gt!=NULL){           i=sprintf(strbuf+offset," gt=\"%s\"",key->gt); 		offset+=i;  	} 
      if(key->lteq!=NULL){         i=sprintf(strbuf+offset," lteq=\"%s\"",key->lteq); 		offset+=i;  	} 
      if(key->gteq!=NULL){         i=sprintf(strbuf+offset," gteq=\"%s\"",key->gteq); 		offset+=i;  	} 
      if(key->select!=NULL){       i=sprintf(strbuf+offset," select=\"%s\"",key->select);	offset+=i;  	} 
      if(key->trap!=NULL){         i=sprintf(strbuf+offset," trap=\"%s\"",key->trap); 		offset+=i;  	} 
      i=sprintf(strbuf+offset,"/>"); offset+=i;
    }
    break;

  case IEEE1888_DATATYPE_OK:
    {
      // ieee1888_OK* OK=(ieee1888_OK*)obj;
      i=sprintf(strbuf+offset,"<OK />");	offset+=i;
    }
    break;

  case IEEE1888_DATATYPE_ERROR:
    {
      ieee1888_error* error=(ieee1888_error*)obj;
      i=sprintf(strbuf+offset,"<error");	offset+=i;
      if(error->type!=NULL){        i=sprintf(strbuf+offset," type=\"%s\"",error->type);         offset+=i;      }
      i=sprintf(strbuf+offset,">"); 	offset+=i;
      if(error->content!=NULL){     i=sprintf(strbuf+offset,"%s",error->content); 		 offset+=i;      }
      i=sprintf(strbuf+offset,"</error>"); 	offset+=i;
    }
    break;

  case IEEE1888_DATATYPE_QUERY:
    {
      ieee1888_query* query=(ieee1888_query*)obj;
      i=sprintf(strbuf+offset,"<query");	offset+=i;
      if(query->id!=NULL){          i=sprintf(strbuf+offset," id=\"%s\"",query->id);             offset+=i;      }
      if(query->type!=NULL){        i=sprintf(strbuf+offset," type=\"%s\"",query->type); 	 offset+=i;      }
      if(query->cursor!=NULL){	    i=sprintf(strbuf+offset," cursor=\"%s\"",query->cursor);     offset+=i;	 }
      if(query->ttl!=-1){           i=sprintf(strbuf+offset," ttl=\"%ld\"",query->ttl);           offset+=i;	 }
      if(query->acceptableSize!=-1){ i=sprintf(strbuf+offset," acceptableSize=\"%ld\"",query->acceptableSize);  offset+=i; }
      if(query->callbackData!=NULL){ i=sprintf(strbuf+offset," callbackData=\"%s\"",query->callbackData);    offset+=i; }
      if(query->callbackControl!=NULL){ i=sprintf(strbuf+offset," callbackControl=\"%s\"",query->callbackControl); offset+=i; }
      i=sprintf(strbuf+offset,">");	offset+=i;
      for(j=0;j<query->n_key;j++){
        i=ieee1888_generateXML((ieee1888_object*)(((ieee1888_key*)(query->key))+j), strbuf+offset, n-offset);
	offset+=i;
      }
      i=sprintf(strbuf+offset,"</query>");	offset+=i;
    }
    break;

  case IEEE1888_DATATYPE_HEADER:
    {
      ieee1888_header* header=(ieee1888_header*)obj;
      i=sprintf(strbuf+offset,"<header>");	offset+=i;
      i=ieee1888_generateXML((ieee1888_object*)header->OK,strbuf+offset,n-offset); offset+=i;
      i=ieee1888_generateXML((ieee1888_object*)header->error,strbuf+offset,n-offset); offset+=i;
      i=ieee1888_generateXML((ieee1888_object*)header->query,strbuf+offset,n-offset); offset+=i;
      i=sprintf(strbuf+offset,"</header>");	offset+=i;
    }
    break;

  case IEEE1888_DATATYPE_VALUE:
    {
      ieee1888_value* value=(ieee1888_value*)obj;
      i=sprintf(strbuf+offset,"<value");	offset+=i;
      if(value->time!=NULL){          i=sprintf(strbuf+offset," time=\"%s\"",value->time);     offset+=i;       }
      i=sprintf(strbuf+offset,">");	offset+=i;
      if(value->content!=NULL){       i=sprintf(strbuf+offset,"%s",value->content);	       offset+=i;	}
      i=sprintf(strbuf+offset,"</value>"); offset+=i;
    }
    break;

  case IEEE1888_DATATYPE_POINT:
    {
      ieee1888_point* point=(ieee1888_point*)obj;
      i=sprintf(strbuf+offset,"<point");	offset+=i;
      if(point->id!=NULL){          i=sprintf(strbuf+offset," id=\"%s\"",point->id);     offset+=i;       }
      i=sprintf(strbuf+offset,">");	offset+=i;
      for(j=0;j<point->n_value;j++){
        i=ieee1888_generateXML((ieee1888_object*)(((ieee1888_value*)(point->value))+j), strbuf+offset, n-offset);
	offset+=i;
      }
      i=sprintf(strbuf+offset,"</point>"); offset+=i;
    }
    break;

  case IEEE1888_DATATYPE_POINTSET:
    {
      ieee1888_pointSet* pointSet=(ieee1888_pointSet*)obj;
      i=sprintf(strbuf+offset,"<pointSet");	offset+=i;
      if(pointSet->id!=NULL){          i=sprintf(strbuf+offset," id=\"%s\"",pointSet->id);     offset+=i;       }
      i=sprintf(strbuf+offset,">");	offset+=i;
      for(j=0;j<pointSet->n_pointSet;j++){
        i=ieee1888_generateXML((ieee1888_object*)(((ieee1888_pointSet*)(pointSet->pointSet))+j), strbuf+offset, n-offset);
	offset+=i;
      }
      for(j=0;j<pointSet->n_point;j++){
        i=ieee1888_generateXML((ieee1888_object*)(((ieee1888_point*)(pointSet->point))+j), strbuf+offset, n-offset);
	offset+=i;
      }
      i=sprintf(strbuf+offset,"</pointSet>");	offset+=i;
    }
    break;


  case IEEE1888_DATATYPE_BODY:
    {
      ieee1888_body* body=(ieee1888_body*)obj;
      i=sprintf(strbuf+offset,"<body>");	offset+=i;
      for(j=0;j<body->n_pointSet;j++){
        i=ieee1888_generateXML((ieee1888_object*)(((ieee1888_pointSet*)(body->pointSet))+j), strbuf+offset, n-offset);
	offset+=i;
      }
      for(j=0;j<body->n_point;j++){
        i=ieee1888_generateXML((ieee1888_object*)(((ieee1888_point*)(body->point))+j), strbuf+offset, n-offset);
	offset+=i;
      }
      i=sprintf(strbuf+offset,"</body>");	offset+=i;
    }
    break;

  case IEEE1888_DATATYPE_TRANSPORT:
    {
      ieee1888_transport* transport=(ieee1888_transport*)obj;
      i=sprintf(strbuf+offset,"<transport xmlns=\"http://gutp.jp/fiap/2009/11/\">");		offset+=i;
      i=ieee1888_generateXML((ieee1888_object*)transport->header,strbuf+offset,n-offset); offset+=i;
      i=ieee1888_generateXML((ieee1888_object*)transport->body,strbuf+offset,n-offset); offset+=i;
      i=sprintf(strbuf+offset,"</transport>"); 	offset+=i;
    }
    break;

  }
  return offset;
}

// #define IEEE1888_SOAP_GEN_ERROR_UNKNOWN_MSG -1
// #define IEEE1888_SOAP_GEN_ERROR_OVERFLOW    -2

int ieee1888_soap_gen(const ieee1888_transport* transport, int message, char* str_soap, int n){

  int len;
  char* p=str_soap;

  strncpy(p,"<?xml version=\'1.0\' encoding=\'UTF-8\'?><soapenv:Envelope xmlns:soapenv=\"http://schemas.xmlsoap.org/soap/envelope/\"><soapenv:Body>",n);
  len=strlen(p); n-=len; p+=len;
  if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

  switch(message){
  case IEEE1888_QUERY_RQ:
    strncpy(p,"<ns2:queryRQ xmlns:ns2=\"http://soap.fiap.org/\">",n); len=strlen(p); n-=len; p+=len; 
    if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

    ieee1888_generateXML((ieee1888_object*)transport,p,n); len=strlen(p); n-=len; p+=len;
    if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

    strncpy(p,"</ns2:queryRQ>",n); len=strlen(p); n-=len; p+=len;
    if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

    break;

  case IEEE1888_QUERY_RS:
    strncpy(p,"<ns2:queryRS xmlns:ns2=\"http://soap.fiap.org/\">",n); len=strlen(p); n-=len; p+=len;
    if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

    ieee1888_generateXML((ieee1888_object*)transport,p,n); len=strlen(p); n-=len; p+=len;
    if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

    strncpy(p,"</ns2:queryRS>",n); len=strlen(p); n-=len; p+=len;
    if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

    break;

  case IEEE1888_DATA_RQ:
    strncpy(p,"<ns2:dataRQ xmlns:ns2=\"http://soap.fiap.org/\">",n); len=strlen(p); n-=len; p+=len;
    if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

    ieee1888_generateXML((ieee1888_object*)transport,p,n); len=strlen(p); n-=len; p+=len;
    if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

    strncpy(p,"</ns2:dataRQ>",n); len=strlen(p); n-=len; p+=len;
    if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

    break;

  case IEEE1888_DATA_RS:
    strncpy(p,"<ns2:dataRS xmlns:ns2=\"http://soap.fiap.org/\">",n); len=strlen(p); n-=len; p+=len;
    if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

    ieee1888_generateXML((ieee1888_object*)transport,p,n); len=strlen(p); n-=len; p+=len;
    if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

    strncpy(p,"</ns2:dataRS>",n); len=strlen(p); n-=len; p+=len;
    if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

    break;

  default:
    return IEEE1888_SOAP_GEN_ERROR_UNKNOWN_MSG;
  }
  
  strncpy(p,"</soapenv:Body></soapenv:Envelope>",n); len=strlen(p); n-=len; p+=len;
  if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

  return strlen(str_soap);
}

int ieee1888_soap_error_gen(const char* error_msg, char* str_soap, int n){

  int len;
  char* p=str_soap;
  strncpy(p,"<?xml version=\'1.0\' encoding=\'UTF-8\'?><soapenv:Envelope xmlns:soapenv=\"http://schemas.xmlsoap.org/soap/envelope/\"><soapenv:Body><soapenv:Fault><faultcode>soapenv:Server</faultcode><faultstring>",n);
  len=strlen(p); n-=len; p+=len;  if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

  strncpy(p,error_msg,n);
  len=strlen(p); n-=len; p+=len;  if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }
 
  strncpy(p,"</faultstring><detail /></soapenv:Fault></soapenv:Body></soapenv:Envelope>",n);
  len=strlen(p); n-=len; p+=len;   if(n<=0){ return IEEE1888_SOAP_GEN_ERROR_OVERFLOW; }

  return strlen(str_soap);
}

/*
int main(int argc, char** argv){

  long l;

  ieee1888_transport* transport=ieee1888_mk_transport();
  transport->header=ieee1888_mk_header();
  transport->body=ieee1888_mk_body();

  ieee1888_pointSet* pointSet=ieee1888_mk_pointSet_array(3);
  pointSet[0].id=ieee1888_mk_uri("http://gutp.jp/Room1/");
  pointSet[1].id=ieee1888_mk_uri("http://gutp.jp/Room2/");
  pointSet[2].id=ieee1888_mk_uri("http://gutp.jp/Room3/");
  transport->body->pointSet=pointSet;
  transport->body->n_pointSet=3;

  ieee1888_point* point=ieee1888_mk_point_array(3);
  point[0].id=ieee1888_mk_uri("http://gutp.jp/Room1/EntranceTemp");
  point[1].id=ieee1888_mk_uri("http://gutp.jp/Room1/EntranceHum");
  point[2].id=ieee1888_mk_uri("http://gutp.jp/Room1/EntranceCO2");
  pointSet[0].point=point;
  pointSet[0].n_point=3;

  ieee1888_value* value=ieee1888_mk_value_array(3);
  value[0].time=ieee1888_mk_string("2011-01-01T00:00:00+09:00");
  value[0].content=ieee1888_mk_string("25.5");
  value[1].time=ieee1888_mk_string("2011-01-02T00:00:00+09:00");
  value[1].content=ieee1888_mk_string("25.6");
  value[2].time=ieee1888_mk_string("2011-01-03T00:00:00+09:00");
  value[2].content=ieee1888_mk_string("25.7");
  point[0].value=value;
  point[0].n_value=3;

  ieee1888_query* query=ieee1888_mk_query();
  query->id=ieee1888_mk_new_uuid();
  query->type=ieee1888_mk_queryType("storage2");
  query->cursor=ieee1888_mk_new_uuid();
  query->ttl=600;
  query->acceptableSize=100;
  query->callbackData=ieee1888_mk_url("http://fiap-dev.gutp.ic.i.u-tokyo.ac.jp/axis2/services/FIAPStorage");
  query->callbackControl=ieee1888_mk_url("http://fiap-dev.gutp.ic.i.u-tokyo.ac.jp/axis2/services/FIAPStorage");
  transport->header->query=query;

  ieee1888_key* key=ieee1888_mk_key_array(3);
  key[0].id=ieee1888_mk_uri("http://gutp.jp/Room1/EntranceTemp");
  key[0].attrName=ieee1888_mk_attrNameType("time");
  key[0].lt=ieee1888_mk_string("2011-01-01T00:00:00+09:00");
  key[0].gt=ieee1888_mk_string("2011-01-01T00:00:00+09:00");
  key[0].eq=ieee1888_mk_string("2011-01-01T00:00:00+09:00");
  key[0].neq=ieee1888_mk_string("2011-01-01T00:00:00+09:00");
  key[0].lteq=ieee1888_mk_string("2011-01-01T00:00:00+09:00");
  key[0].gteq=ieee1888_mk_string("2011-01-01T00:00:00+09:00");
  key[0].select=ieee1888_mk_selectType("maximum");
  key[0].trap=ieee1888_mk_trapType("changed");
  key[1].id=ieee1888_mk_uri("http://gutp.jp/Room1/EntranceHum");
  key[1].attrName=ieee1888_mk_attrNameType("value");
  key[1].lt=ieee1888_mk_string("2011-01-02T00:00:00+09:00");
  key[1].gt=ieee1888_mk_string("2011-01-02T00:00:00+09:00");
  key[1].eq=ieee1888_mk_string("2011-01-02T00:00:00+09:00");
  key[1].neq=ieee1888_mk_string("2011-01-02T00:00:00+09:00");
  key[1].lteq=ieee1888_mk_string("2011-01-02T00:00:00+09:00");
  key[1].gteq=ieee1888_mk_string("2011-01-02T00:00:00+09:00");
  key[1].select=ieee1888_mk_selectType("maximum");
  key[1].trap=ieee1888_mk_trapType("changed");
  key[2].id=ieee1888_mk_uri("http://gutp.jp/Room1/EntranceCO2");
  key[2].attrName=ieee1888_mk_attrNameType("time");
  key[2].lt=ieee1888_mk_string("2011-01-03T00:00:00+09:00");
  key[2].gt=ieee1888_mk_string("2011-01-03T00:00:00+09:00");
  key[2].eq=ieee1888_mk_string("2011-01-03T00:00:00+09:00");
  key[2].neq=ieee1888_mk_string("2011-01-03T00:00:00+09:00");
  key[2].lteq=ieee1888_mk_string("2011-01-03T00:00:00+09:00");
  key[2].gteq=ieee1888_mk_string("2011-01-03T00:00:00+09:00");
  key[2].select=ieee1888_mk_selectType("minimum");
  key[2].trap=ieee1888_mk_trapType("changed");
  
  query->key=key;
  query->n_key=3;

  ieee1888_error* error=ieee1888_mk_error();
  error->type=ieee1888_mk_string("NOT_AUTHORIZED");
  error->content=ieee1888_mk_string("Not Authorized");
  transport->header->error=error;

  ieee1888_OK* ok=ieee1888_mk_OK();
  transport->header->OK=ok;

  // ieee1888_dump_objects((ieee1888_object*)transport);
  if(transport!=NULL){
    char buffer[100000];
    ieee1888_generateXML((ieee1888_object*)transport,buffer,100000);

    ieee1888_destroy_objects((ieee1888_object*)transport);
    free(transport);

    printf(buffer);
  }
  return 0;
}

*/
