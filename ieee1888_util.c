
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ieee1888.h"

ieee1888_key*       ieee1888_add_key_to_array(ieee1888_key* key_array, int n_key, ieee1888_key* add){

  if(n_key<0){
    return NULL;
  }
  ieee1888_key* array=(ieee1888_key*)calloc(sizeof(ieee1888_key),n_key+1);
  if(array!=NULL){
    memcpy(array,key_array,sizeof(ieee1888_key)*n_key);
    memcpy(&array[n_key],add,sizeof(ieee1888_key));
    return array;
  }
  return NULL;

}

ieee1888_value*     ieee1888_add_value_to_array(ieee1888_value* value_array, int n_value, ieee1888_value* add){
  
  if(n_value<0){
    return NULL;
  }
  ieee1888_value* array=(ieee1888_value*)calloc(sizeof(ieee1888_value),n_value+1);
  if(array!=NULL){
    memcpy(array,value_array,sizeof(ieee1888_value)*n_value);
    memcpy(&array[n_value],add,sizeof(ieee1888_value));
    return array;
  }
  return NULL;

}

ieee1888_point*     ieee1888_add_point_to_array(ieee1888_point* point_array, int n_point, ieee1888_point* add){

  if(n_point<0){
    return NULL;
  }
  ieee1888_point* array=(ieee1888_point*)calloc(sizeof(ieee1888_point),n_point+1);
  if(array!=NULL){
    memcpy(array,point_array,sizeof(ieee1888_point)*n_point);
    memcpy(&array[n_point],add,sizeof(ieee1888_point));
    return array;
  }
  return NULL;

}

ieee1888_pointSet*  ieee1888_add_pointSet_to_array(ieee1888_pointSet* pointSet_array, int n_pointSet, ieee1888_pointSet* add){
  
  if(n_pointSet<0){
    return NULL;
  }
  ieee1888_pointSet* array=(ieee1888_pointSet*)calloc(sizeof(ieee1888_pointSet),n_pointSet+1);
  if(array!=NULL){
    memcpy(array,pointSet_array,sizeof(ieee1888_pointSet)*n_pointSet);
    memcpy(&array[n_pointSet],add,sizeof(ieee1888_pointSet));
    return array;
  }
  return NULL;

}

// error generators
ieee1888_error* ieee1888_mk_error_query_not_supported(const char* msg){
  ieee1888_error* error=ieee1888_mk_error();
  error->type=ieee1888_mk_string("QUERY_NOT_SUPPORTED");
  if(msg!=NULL){
    error->content=ieee1888_mk_string(msg);
  }
  return error;
}

ieee1888_error* ieee1888_mk_error_invalid_cursor(const char* msg){
  ieee1888_error* error=ieee1888_mk_error();
  error->type=ieee1888_mk_string("INVALID_CURSOR");
  if(msg!=NULL){
    error->content=ieee1888_mk_string(msg);
  }
  return error;
}

ieee1888_error* ieee1888_mk_error_point_not_found(const char* msg){
  ieee1888_error* error=ieee1888_mk_error();
  error->type=ieee1888_mk_string("POINT_NOT_FOUND");
  if(msg!=NULL){
    error->content=ieee1888_mk_string(msg);
  }
  return error;
}

ieee1888_error* ieee1888_mk_error_forbidden(const char* msg){
  ieee1888_error* error=ieee1888_mk_error();
  error->type=ieee1888_mk_string("FORBIDDEN");
  if(msg!=NULL){
    error->content=ieee1888_mk_string(msg);
  }
  return error;
}

ieee1888_error* ieee1888_mk_error_value_time_not_specified(const char* msg){
  ieee1888_error* error=ieee1888_mk_error();
  error->type=ieee1888_mk_string("VALUE_TIME_NOT_SPECIFIED");
  if(msg!=NULL){
    error->content=ieee1888_mk_string(msg);
  }
  return error;
}

ieee1888_error* ieee1888_mk_error_too_big_request(const char* msg){
  ieee1888_error* error=ieee1888_mk_error();
  error->type=ieee1888_mk_string("TOO_BIG_REQUEST");
  if(msg!=NULL){
    error->content=ieee1888_mk_string(msg);
  }
  return error;
}

ieee1888_error* ieee1888_mk_error_too_many_keys(const char* msg){
  ieee1888_error* error=ieee1888_mk_error();
  error->type=ieee1888_mk_string("TOO_MANY_KEYS");
  if(msg!=NULL){
    error->content=ieee1888_mk_string(msg);
  }
  return error;
}

ieee1888_error* ieee1888_mk_error_too_many_values(const char* msg){
  ieee1888_error* error=ieee1888_mk_error();
  error->type=ieee1888_mk_string("TOO_MANY_VALUES");
  if(msg!=NULL){
    error->content=ieee1888_mk_string(msg);
  }
  return error;
}

ieee1888_error* ieee1888_mk_error_invalid_request(const char* msg){
  ieee1888_error* error=ieee1888_mk_error();
  error->type=ieee1888_mk_string("INVALID_REQUEST");
  if(msg!=NULL){
    error->content=ieee1888_mk_string(msg);
  }
  return error;
}

ieee1888_error* ieee1888_mk_error_server_error(const char* msg){
  ieee1888_error* error=ieee1888_mk_error();
  error->type=ieee1888_mk_string("SERVER_ERROR");
  if(msg!=NULL){
    error->content=ieee1888_mk_string(msg);
  }
  return error;
}

ieee1888_error* ieee1888_mk_error_unknown_error(const char* msg){
  ieee1888_error* error=ieee1888_mk_error();
  error->type=ieee1888_mk_string("UNKNOWN_ERROR");
  if(msg!=NULL){
    error->content=ieee1888_mk_string(msg);
  }
  return error;
}


void ieee1888_dump_objects_impl(ieee1888_object* obj, int indent){

  if(obj==NULL){
    return;
  }

  int i;
  char indent_buffer[50];
  char next_indent_buffer[50];
  for(i=0;i<50;i++){
    indent_buffer[i]=' ';
    next_indent_buffer[i]=' ';
  }
  indent_buffer[indent]=0;
  next_indent_buffer[indent+2]=0;

  switch(obj->dtype){
 
  case IEEE1888_DATATYPE_KEY:
    {
      ieee1888_key* key=(ieee1888_key*)obj;
      printf(indent_buffer); printf("key {\n");
      if(key->id==NULL){
        printf(next_indent_buffer); printf("id : null\n");
      }else{
        printf(next_indent_buffer); printf("id : \"%s\"\n",key->id);
      }
      if(key->attrName==NULL){
        printf(next_indent_buffer); printf("attrName : null\n");
      }else{
        printf(next_indent_buffer); printf("attrName : \"%s\"\n",key->attrName);
      }
      if(key->eq==NULL){
        printf(next_indent_buffer); printf("eq : null\n");
      }else{
        printf(next_indent_buffer); printf("eq : \"%s\"\n",key->eq);
      }
      if(key->neq==NULL){
        printf(next_indent_buffer); printf("neq : null\n");
      }else{
        printf(next_indent_buffer); printf("neq : \"%s\"\n",key->neq);
      }
      if(key->lt==NULL){
        printf(next_indent_buffer); printf("lt : null\n");
      }else{
        printf(next_indent_buffer); printf("lt : \"%s\"\n",key->lt);
      }
      if(key->gt==NULL){
        printf(next_indent_buffer); printf("gt : null\n");
      }else{
        printf(next_indent_buffer); printf("gt : \"%s\"\n",key->gt);
      }
      if(key->lteq==NULL){
        printf(next_indent_buffer); printf("lteq : null\n");
      }else{
        printf(next_indent_buffer); printf("lteq : \"%s\"\n",key->lteq);
      }
      if(key->gteq==NULL){
        printf(next_indent_buffer); printf("gteq : null\n");
      }else{
        printf(next_indent_buffer); printf("gteq : \"%s\"\n",key->gteq);
      }
      if(key->select==NULL){
        printf(next_indent_buffer); printf("select : null\n");
      }else{
        printf(next_indent_buffer); printf("select : \"%s\"\n",key->select);
      }
      if(key->trap==NULL){
        printf(next_indent_buffer); printf("trap : null\n");
      }else{
        printf(next_indent_buffer); printf("trap : \"%s\"\n",key->trap);
      }
      
      printf(indent_buffer); printf("}\n");
    }
    break;

  case IEEE1888_DATATYPE_OK:
    {
      // ieee1888_OK* OK=(ieee1888_OK*)obj;
      printf(indent_buffer); printf("OK {\n");
      printf(indent_buffer); printf("}\n");
    }
    break;

  case IEEE1888_DATATYPE_ERROR:
    {
      ieee1888_error* error=(ieee1888_error*)obj;
      printf(indent_buffer); printf("error {\n");
      if(error->type==NULL){
        printf(next_indent_buffer); printf("type : null\n");
      }else{
        printf(next_indent_buffer); printf("type : \"%s\"\n",error->type);
      }
      if(error->content==NULL){
        printf(next_indent_buffer); printf("content : null\n");
      }else{
        printf(next_indent_buffer); printf("content : \"%s\"\n",error->content);
      }
      printf(indent_buffer); printf("}\n");
    }
    break;

  case IEEE1888_DATATYPE_QUERY:
    {
      ieee1888_query* query=(ieee1888_query*)obj;
      printf(indent_buffer); printf("query {\n");
      if(query->id==NULL){
        printf(next_indent_buffer); printf("id : null\n");
      }else{
        printf(next_indent_buffer); printf("id : \"%s\"\n",query->id);
      }
      if(query->type==NULL){
        printf(next_indent_buffer); printf("type : null\n");
      }else{
        printf(next_indent_buffer); printf("type : \"%s\"\n",query->type);
      }
      if(query->cursor==NULL){
        printf(next_indent_buffer); printf("cursor : null\n");
      }else{
        printf(next_indent_buffer); printf("cursor : \"%s\"\n",query->cursor);
      }
      printf(next_indent_buffer); printf("ttl : %ld\n",query->ttl);
      printf(next_indent_buffer); printf("acceptableSize : %ld\n",query->acceptableSize);
      if(query->callbackData==NULL){
        printf(next_indent_buffer); printf("callbackData : null\n");
      }else{
        printf(next_indent_buffer); printf("callbackData : \"%s\"\n",query->callbackData);
      }
      if(query->callbackControl==NULL){
        printf(next_indent_buffer); printf("callbackControl : null\n");
      }else{
        printf(next_indent_buffer); printf("callbackControl : \"%s\"\n",query->callbackControl);
      }

      for(i=0;i<query->n_key;i++){
        ieee1888_dump_objects_impl((ieee1888_object*)(((ieee1888_key*)(query->key))+i),indent+2);
      }

      printf(indent_buffer); printf("}\n");
    }
    break;

  case IEEE1888_DATATYPE_HEADER:
    {
      ieee1888_header* header=(ieee1888_header*)obj;
      printf(indent_buffer); printf("header {\n");
      ieee1888_dump_objects_impl((ieee1888_object*)header->OK,indent+2);
      ieee1888_dump_objects_impl((ieee1888_object*)header->error,indent+2);
      ieee1888_dump_objects_impl((ieee1888_object*)header->query,indent+2);
      printf(indent_buffer); printf("}\n");
    }
    break;

  case IEEE1888_DATATYPE_VALUE:
    {
      ieee1888_value* value=(ieee1888_value*)obj;
      printf(indent_buffer); printf("value {\n");
      if(value->time==NULL){
      	printf(next_indent_buffer); printf("time : null\n");
      }else{
      	printf(next_indent_buffer); printf("time : %s\n",value->time);
      }
      if(value->content==NULL){
        printf(next_indent_buffer); printf("content : null\n");
      }else{
        printf(next_indent_buffer); printf("content : \"%s\"\n",value->content);
      }
      printf(indent_buffer); printf("}\n");
    }
    break;

  case IEEE1888_DATATYPE_POINT:
    {
      ieee1888_point* point=(ieee1888_point*)obj;
      printf(indent_buffer); printf("point {\n");
      if(point->id==NULL){
        printf(next_indent_buffer); printf("id : null\n");
      }else{
        printf(next_indent_buffer); printf("id : \"%s\"\n",point->id);
      }
      for(i=0;i<point->n_value;i++){
        ieee1888_dump_objects_impl((ieee1888_object*)(((ieee1888_value*)(point->value))+i),indent+2);
      }
      printf(indent_buffer); printf("}\n");
    }
    break;

  case IEEE1888_DATATYPE_POINTSET:
    {
      ieee1888_pointSet* pointSet=(ieee1888_pointSet*)obj;
      printf(indent_buffer); printf("pointSet {\n");
      if(pointSet->id==NULL){
        printf(next_indent_buffer); printf("id : null\n");
      }else{
        printf(next_indent_buffer); printf("id : \"%s\"\n",pointSet->id);
      }
      for(i=0;i<pointSet->n_pointSet;i++){
        ieee1888_dump_objects_impl((ieee1888_object*)(((ieee1888_pointSet*)(pointSet->pointSet))+i),indent+2);
      }
      for(i=0;i<pointSet->n_point;i++){
        ieee1888_dump_objects_impl((ieee1888_object*)(((ieee1888_point*)(pointSet->point))+i),indent+2);
      }
      printf(indent_buffer); printf("}\n");
    }
    break;


  case IEEE1888_DATATYPE_BODY:
    {
      ieee1888_body* body=(ieee1888_body*)obj;
      printf(indent_buffer); printf("body {\n");
       for(i=0;i<body->n_pointSet;i++){
        ieee1888_dump_objects_impl((ieee1888_object*)(((ieee1888_pointSet*)body->pointSet)+i),indent+2);
      }
      for(i=0;i<body->n_point;i++){
        ieee1888_dump_objects_impl((ieee1888_object*)(((ieee1888_point*)(body->point))+i),indent+2);
      }
      printf(indent_buffer); printf("}\n");
    }
    break;

  case IEEE1888_DATATYPE_TRANSPORT:
    {
      ieee1888_transport* transport=(ieee1888_transport*)obj;
      printf(indent_buffer); printf("transport {\n");
      ieee1888_dump_objects_impl((ieee1888_object*)transport->header,indent+2);
      ieee1888_dump_objects_impl((ieee1888_object*)transport->body,indent+2);
      printf(indent_buffer); printf("}\n");
    }
    break;

  }
}

void ieee1888_dump_objects(ieee1888_object* obj){
  ieee1888_dump_objects_impl(obj,0);
}

/*
int main(int argc, char* argv){

  int i;
  char buf[10];
  ieee1888_pointSet* p=NULL;
  ieee1888_pointSet* q=NULL;
  for(i=0;i<100;i++){
    sprintf(buf,"%d",i);
    ieee1888_pointSet* point=ieee1888_mk_pointSet();
    point->id=ieee1888_mk_string(buf);
    q=ieee1888_add_pointSet_to_array(p,i,point);

    if(p!=NULL){
      free(p);
    }
    p=q;
  }

  ieee1888_body* body=ieee1888_mk_body();
  body->pointSet=p;
  body->n_pointSet=100;

  ieee1888_dump_objects((ieee1888_object*)body);

  return 0;
}

*/

