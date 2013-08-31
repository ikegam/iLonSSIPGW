
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ieee1888.h"

char* ieee1888_mk_string(const char* str){
  int len=strlen(str);
  char* p=(char*)malloc(len+1);
  strcpy(p,str);
  return p;
}

ieee1888_time* ieee1888_mk_time_from_string(const char* time){
  // TODO: check the schema of the "time"
  return ieee1888_mk_string(time);
}

ieee1888_time*      ieee1888_mk_time(time_t time){

  struct tm t;
  localtime_r(&time,&t);
  long w_timezone=timezone;
  char tzoffset[8];

  if(w_timezone<0){
    w_timezone=-w_timezone;
    tzoffset[0]='+';
  }else{
    tzoffset[0]='-';
  }
  sprintf(&tzoffset[1],"%02ld:%02ld",w_timezone/3600,(w_timezone/60)%60);

  char buffer[30];
  sprintf(buffer,"%04d-%02d-%02dT%02d:%02d:%02d%s",
              t.tm_year+1900,
	      t.tm_mon+1,
	      t.tm_mday,
	      t.tm_hour,
	      t.tm_min,
	      t.tm_sec,
	      tzoffset);

  return ieee1888_mk_string(buffer);
}

ieee1888_time*      ieee1888_mk_time_with_tz(time_t time, long timezone_offset){
  
  struct tm t;
  localtime_r(&time,&t);
  long w_timezone=timezone_offset;
  char tzoffset[8];

  if(w_timezone<0){
    w_timezone=-w_timezone;
    tzoffset[0]='+';
  }else{
    tzoffset[0]='-';
  }
  sprintf(&tzoffset[1],"%02ld:%02ld",w_timezone/3600,(w_timezone/60)%60);

  char buffer[30];
  sprintf(buffer,"%04d-%02d-%02dT%02d:%02d:%02d%s",
              t.tm_year+1900,
	      t.tm_mon+1,
	      t.tm_mday,
	      t.tm_hour,
	      t.tm_min,
	      t.tm_sec,
	      tzoffset);

  return ieee1888_mk_string(buffer);
}

ieee1888_uri* ieee1888_mk_uri(const char* uri){
  // TODO: check the schema of the "uri"
  return ieee1888_mk_string(uri);
}

ieee1888_url* ieee1888_mk_url(const char* url){
  // TODO: check the schema of the "url"
  return ieee1888_mk_string(url);
}

ieee1888_uuid* ieee1888_mk_uuid(const char* uuid){
  // TODO: check the schema of the "uuid"
  return ieee1888_mk_string(uuid);
}

ieee1888_uuid* ieee1888_mk_new_uuid(){
  // generate random uuid

  char new_uuid[38];
  sprintf(new_uuid,"%04x%04x-%04x-%04x-%04x-%04x%04x%04x",
    1+(int)(((double)0xffff)*(rand()/(RAND_MAX+1.0))),
    1+(int)(((double)0xffff)*(rand()/(RAND_MAX+1.0))),
    1+(int)(((double)0xffff)*(rand()/(RAND_MAX+1.0))),
    (1+(int)(((double)0xffff)*(rand()/(RAND_MAX+1.0))))|0x4000,
    (1+(int)(((double)0xffff)*(rand()/(RAND_MAX+1.0))))|0x8000,
    1+(int)(((double)0xffff)*(rand()/(RAND_MAX+1.0))),
    1+(int)(((double)0xffff)*(rand()/(RAND_MAX+1.0))),
    1+(int)(((double)0xffff)*(rand()/(RAND_MAX+1.0))));

  return ieee1888_mk_string(new_uuid);
}

ieee1888_queryType* ieee1888_mk_queryType(const char* queryType){
  if(strcmp(queryType,"storage")==0){
    return ieee1888_mk_string("storage");
  }else if(strcmp(queryType,"stream")==0){
    return ieee1888_mk_string("stream");
  }
  return NULL;
}

ieee1888_attrNameType* ieee1888_mk_attrNameType(const char* attrName){
  if(strcmp(attrName,"time")==0){
    return ieee1888_mk_string("time");
  }else if(strcmp(attrName,"value")==0){
    return ieee1888_mk_string("value");
  }
  return NULL;
}

ieee1888_selectType* ieee1888_mk_selectType(const char* select){
  if(strcmp(select,"maximum")==0){
    return ieee1888_mk_string("maximum");
  }else if(strcmp(select,"minimum")==0){
    return ieee1888_mk_string("minimum");
  }
  return NULL;
}

ieee1888_trapType* ieee1888_mk_trapType(const char* trap){
  if(strcmp(trap,"changed")==0){
    return ieee1888_mk_string("changed");
  }
  return NULL;
}

ieee1888_key* ieee1888_mk_key(){
	ieee1888_object* obj=(ieee1888_object*)calloc(sizeof(ieee1888_key),1);
	if(obj!=NULL){
		obj->dtype=IEEE1888_DATATYPE_KEY;
		return (ieee1888_key*)obj;
	}
	return NULL;
}

ieee1888_key* ieee1888_mk_key_array(int n){
	ieee1888_key* array=(ieee1888_key*)calloc(sizeof(ieee1888_key),n);
	if(array!=NULL){
	        int i;
		for(i=0;i<n;i++){
   			array[i].dtype=IEEE1888_DATATYPE_KEY;
		}
		return array;
	}
	return NULL;
}

ieee1888_OK* ieee1888_mk_OK(){
	ieee1888_object* obj=(ieee1888_object*)calloc(sizeof(ieee1888_OK),1);
	if(obj!=NULL){
		obj->dtype=IEEE1888_DATATYPE_OK;
		return (ieee1888_OK*)obj;
	}
	return NULL;
}

ieee1888_error* ieee1888_mk_error(){
	ieee1888_object* obj=(ieee1888_object*)calloc(sizeof(ieee1888_error),1);
	if(obj!=NULL){
		obj->dtype=IEEE1888_DATATYPE_ERROR;
		return (ieee1888_error*)obj;
	}
	return NULL;
}

ieee1888_query* ieee1888_mk_query(){
	ieee1888_object* obj=(ieee1888_object*)calloc(sizeof(ieee1888_query),1);
	if(obj!=NULL){
		obj->dtype=IEEE1888_DATATYPE_QUERY;
		((ieee1888_query*)obj)->ttl=-1;
		((ieee1888_query*)obj)->acceptableSize=-1;
		return (ieee1888_query*)obj;
	}
	return NULL;
}

ieee1888_header* ieee1888_mk_header(){
	ieee1888_object* obj=(ieee1888_object*)calloc(sizeof(ieee1888_header),1);
	if(obj!=NULL){
		obj->dtype=IEEE1888_DATATYPE_HEADER;
		return (ieee1888_header*)obj;
	}
	return NULL;
}

ieee1888_value* ieee1888_mk_value(){
	ieee1888_object* obj=(ieee1888_object*)calloc(sizeof(ieee1888_value),1);
	if(obj!=NULL){
		obj->dtype=IEEE1888_DATATYPE_VALUE;
		return (ieee1888_value*)obj;
	}
	return NULL;
}

ieee1888_value* ieee1888_mk_value_array(int n){
	ieee1888_value* array=(ieee1888_value*)calloc(sizeof(ieee1888_value),n);
	if(array!=NULL){
		int i;
		for(i=0;i<n;i++){
			array[i].dtype=IEEE1888_DATATYPE_VALUE;
		}
		return array;
	}
	return NULL;
}

ieee1888_point* ieee1888_mk_point(){
	ieee1888_object* obj=(ieee1888_object*)calloc(sizeof(ieee1888_point),1);
	if(obj!=NULL){
		obj->dtype=IEEE1888_DATATYPE_POINT;
		return (ieee1888_point*)obj;
	}
	return NULL;
}

ieee1888_point* ieee1888_mk_point_array(int n){
	ieee1888_point* array=(ieee1888_point*)calloc(sizeof(ieee1888_point),n);
	if(array!=NULL){
	        int i;
		for(i=0;i<n;i++){
			array[i].dtype=IEEE1888_DATATYPE_POINT;
		}
		return array;
	}
	return NULL;
}

ieee1888_pointSet* ieee1888_mk_pointSet(){
	ieee1888_object* obj=(ieee1888_object*)calloc(sizeof(ieee1888_pointSet),1);
	if(obj!=NULL){
		obj->dtype=IEEE1888_DATATYPE_POINTSET;
		return (ieee1888_pointSet*)obj;
	}
	return NULL;
}

ieee1888_pointSet* ieee1888_mk_pointSet_array(int n){
	ieee1888_pointSet* array=(ieee1888_pointSet*)calloc(sizeof(ieee1888_pointSet),n);
	if(array!=NULL){
	        int i;
		for(i=0;i<n;i++){
			array[i].dtype=IEEE1888_DATATYPE_POINTSET;
		}
		return array;
	}
	return NULL;
}

ieee1888_body* ieee1888_mk_body(){
	ieee1888_object* obj=(ieee1888_object*)calloc(sizeof(ieee1888_body),1);
	if(obj!=NULL){
		obj->dtype=IEEE1888_DATATYPE_BODY;
		return (ieee1888_body*)obj;
	}
	return NULL;
}

ieee1888_transport* ieee1888_mk_transport(){
	ieee1888_object* obj=(ieee1888_object*)calloc(sizeof(ieee1888_transport),1);
	if(obj!=NULL){
		obj->dtype=IEEE1888_DATATYPE_TRANSPORT;
		return (ieee1888_transport*)obj;
	}
	return NULL;
}

ieee1888_object* ieee1888_clone_objects(ieee1888_object* obj, int n_array){
  
  if(obj==NULL){
    return NULL;
  }

  // int i;
  switch(obj->dtype){
 
  case IEEE1888_DATATYPE_KEY:
    {
      int i;
      ieee1888_key* src=(ieee1888_key*)obj;
      ieee1888_key* clone=ieee1888_mk_key_array(n_array);
      ieee1888_key *p, *q;
      for(i=0, p=src, q=clone; i<n_array; i++, p++, q++){
        if(p->id!=NULL){ 	q->id=ieee1888_mk_uri(p->id);                        }
        if(p->attrName!=NULL){  q->attrName=ieee1888_mk_attrNameType(p->attrName);   }
        if(p->eq!=NULL){ 	q->eq=ieee1888_mk_string(p->eq);                     }
        if(p->neq!=NULL){ 	q->neq=ieee1888_mk_string(p->neq);                   }
        if(p->lt!=NULL){ 	q->lt=ieee1888_mk_string(p->lt);                     }
        if(p->gt!=NULL){ 	q->gt=ieee1888_mk_string(p->gt);                     }
        if(p->lteq!=NULL){ 	q->lteq=ieee1888_mk_string(p->lteq);                 }
        if(p->gteq!=NULL){ 	q->gteq=ieee1888_mk_string(p->gteq);                 }
        if(p->select!=NULL){ 	q->select=ieee1888_mk_selectType(p->select);         }
        if(p->trap!=NULL){ 	q->trap=ieee1888_mk_trapType(p->trap);               }
      }
      return (ieee1888_object*)clone;
    }
    break;

  case IEEE1888_DATATYPE_OK:
    {
      // ieee1888_OK* src=(ieee1888_OK*)obj;
      ieee1888_OK* clone=ieee1888_mk_OK();
      return (ieee1888_object*)clone;
    }
    break;

  case IEEE1888_DATATYPE_ERROR:
    {
      ieee1888_error* src=(ieee1888_error*)obj;
      ieee1888_error* clone=ieee1888_mk_error();
      if(src->type!=NULL){ clone->type=ieee1888_mk_string(src->type); }
      if(src->content!=NULL){ clone->content=ieee1888_mk_string(src->content); }
      return (ieee1888_object*)clone;
    }
    break;

  case IEEE1888_DATATYPE_QUERY:
    {
      ieee1888_query* src=(ieee1888_query*)obj;
      ieee1888_query* clone=ieee1888_mk_query();
      if(src->id!=NULL){              clone->id=ieee1888_mk_uuid(src->id);  }
      if(src->type!=NULL){            clone->type=ieee1888_mk_queryType(src->type);  }
      if(src->cursor!=NULL){          clone->cursor=ieee1888_mk_uuid(src->cursor);  }
      clone->ttl=src->ttl;
      clone->acceptableSize=src->acceptableSize;
      if(src->callbackData!=NULL){    clone->callbackData=ieee1888_mk_url(src->callbackData);  }
      if(src->callbackControl!=NULL){ clone->callbackControl=ieee1888_mk_url(src->callbackControl);  }
      if(src->key!=NULL && src->n_key>0){
        clone->n_key=src->n_key;
	clone->key=(ieee1888_key*)ieee1888_clone_objects((ieee1888_object*)src->key,src->n_key);
      }
      return (ieee1888_object*)clone;
    }
    break;

  case IEEE1888_DATATYPE_HEADER:
    {
      ieee1888_header* src=(ieee1888_header*)obj;
      ieee1888_header* clone=ieee1888_mk_header();
      if(src->OK!=NULL){
        clone->OK=(ieee1888_OK*)ieee1888_clone_objects((ieee1888_object*)src->OK,1);
      }
      if(src->error!=NULL){
        clone->error=(ieee1888_error*)ieee1888_clone_objects((ieee1888_object*)src->error,1);
      }
      if(src->query!=NULL){
        clone->query=(ieee1888_query*)ieee1888_clone_objects((ieee1888_object*)src->query,1);
      }
      return (ieee1888_object*)clone;
    }
    break;

  case IEEE1888_DATATYPE_VALUE:
    {
      int i;
      ieee1888_value* src=(ieee1888_value*)obj;
      ieee1888_value* clone=ieee1888_mk_value_array(n_array);
      ieee1888_value *p, *q;
      for(i=0, p=src, q=clone; i<n_array; i++, p++, q++){
        if(p->time!=NULL){    q->time=ieee1888_mk_time_from_string(p->time); }
        if(p->content!=NULL){ q->content=ieee1888_mk_string(p->content);     }
      }
      return (ieee1888_object*)clone;
    }
    break;

  case IEEE1888_DATATYPE_POINT:
    {
      int i;
      ieee1888_point* src=(ieee1888_point*)obj;
      ieee1888_point* clone=ieee1888_mk_point_array(n_array);
      ieee1888_point *p, *q;
      for(i=0, p=src, q=clone; i<n_array; i++, p++, q++){
        if(p->id!=NULL){ q->id=ieee1888_mk_uri(p->id); }
        if(p->value!=NULL && p->n_value>0){
          q->n_value=p->n_value;
	  q->value=(ieee1888_value*)ieee1888_clone_objects((ieee1888_object*)p->value,p->n_value);
        }
      }
      return (ieee1888_object*)clone;
    }
    break;

  case IEEE1888_DATATYPE_POINTSET:
    { 
      int i;
      ieee1888_pointSet* src=(ieee1888_pointSet*)obj;
      ieee1888_pointSet* clone=ieee1888_mk_pointSet_array(n_array);
      ieee1888_pointSet *p, *q;
      for(i=0, p=src, q=clone; i<n_array; i++, p++, q++){
        if(p->id!=NULL){ q->id=ieee1888_mk_uri(p->id); }
        if(p->pointSet!=NULL && p->n_pointSet>0){
          q->n_pointSet=p->n_pointSet;
	  q->pointSet=(ieee1888_pointSet*)ieee1888_clone_objects((ieee1888_object*)p->pointSet,p->n_pointSet);
        }
        if(p->point!=NULL && p->n_point>0){
          q->n_point=p->n_point;
	  q->point=(ieee1888_point*)ieee1888_clone_objects((ieee1888_object*)p->point,p->n_point);
        }
      }
      return (ieee1888_object*)clone;
    }
    break;


  case IEEE1888_DATATYPE_BODY:
    {
      ieee1888_body* src=(ieee1888_body*)obj;
      ieee1888_body* clone=ieee1888_mk_body();
      if(src->pointSet!=NULL && src->n_pointSet>0){
        clone->n_pointSet=src->n_pointSet;
	clone->pointSet=(ieee1888_pointSet*)ieee1888_clone_objects((ieee1888_object*)src->pointSet,src->n_pointSet);
      }
      if(src->point!=NULL && src->n_point>0){
        clone->n_point=src->n_point;
	clone->point=(ieee1888_point*)ieee1888_clone_objects((ieee1888_object*)src->point,src->n_point);
      }
      return (ieee1888_object*)clone;
    }
    break;

  case IEEE1888_DATATYPE_TRANSPORT:
    {
      ieee1888_transport* src=(ieee1888_transport*)obj;
      ieee1888_transport* clone=ieee1888_mk_transport();
      if(src->header!=NULL){
        clone->header=(ieee1888_header*)ieee1888_clone_objects((ieee1888_object*)src->header,1);
      }
      if(src->body!=NULL){
        clone->body=(ieee1888_body*)ieee1888_clone_objects((ieee1888_object*)src->body,1);
      }
      return (ieee1888_object*)clone;
    }
    break;
  }

  return NULL;
}

void ieee1888_destroy_objects(ieee1888_object* obj){

  if(obj==NULL){
    return;
  }

  int i;
  switch(obj->dtype){
 
  case IEEE1888_DATATYPE_KEY:
    {
      ieee1888_key* key=(ieee1888_key*)obj;
      if(key->id!=NULL){ 	free(key->id);         }
      if(key->attrName!=NULL){  free(key->attrName);   }
      if(key->eq!=NULL){ 	free(key->eq);	       }
      if(key->neq!=NULL){ 	free(key->neq);	       }
      if(key->lt!=NULL){ 	free(key->lt);	       }
      if(key->gt!=NULL){ 	free(key->gt);	       }
      if(key->lteq!=NULL){ 	free(key->lteq);       }
      if(key->gteq!=NULL){ 	free(key->gteq);       }
      if(key->select!=NULL){ 	free(key->select);     }
      if(key->trap!=NULL){ 	free(key->trap);       }
    }
    break;

  case IEEE1888_DATATYPE_OK:
    {
      // ieee1888_OK* OK=(ieee1888_OK*)obj;
    }
    break;

  case IEEE1888_DATATYPE_ERROR:
    {
      ieee1888_error* error=(ieee1888_error*)obj;
      if(error->type!=NULL){	free(error->type); 	}
      if(error->content!=NULL){ free(error->content);	}
    }
    break;

  case IEEE1888_DATATYPE_QUERY:
    {
      ieee1888_query* query=(ieee1888_query*)obj;
      if(query->id!=NULL){	free(query->id);	}
      if(query->type!=NULL){	free(query->type);	}
      if(query->cursor!=NULL){	free(query->cursor);	}
      if(query->callbackData!=NULL){	free(query->callbackData);	}
      if(query->callbackControl!=NULL){	free(query->callbackControl);	}
      if(query->key!=NULL){
        for(i=0;i<query->n_key;i++){
          ieee1888_destroy_objects((ieee1888_object*)(((ieee1888_key*)(query->key))+i));
        }
	free(query->key);
      }
    }
    break;

  case IEEE1888_DATATYPE_HEADER:
    {
      ieee1888_header* header=(ieee1888_header*)obj;
      if(header->OK!=NULL){
        ieee1888_destroy_objects((ieee1888_object*)header->OK);
	free(header->OK);
      }
      if(header->error!=NULL){
        ieee1888_destroy_objects((ieee1888_object*)header->error);
	free(header->error);
      }
      if(header->query!=NULL){
        ieee1888_destroy_objects((ieee1888_object*)header->query);
	free(header->query);
      }
    }
    break;

  case IEEE1888_DATATYPE_VALUE:
    {
      ieee1888_value* value=(ieee1888_value*)obj;
      if(value->time!=NULL){	free(value->time);	}
      if(value->content!=NULL){	free(value->content);	}
    }
    break;

  case IEEE1888_DATATYPE_POINT:
    {
      ieee1888_point* point=(ieee1888_point*)obj;
      if(point->id!=NULL){	free(point->id);	}
      if(point->value!=NULL){
        for(i=0;i<point->n_value;i++){
          ieee1888_destroy_objects((ieee1888_object*)(((ieee1888_value*)(point->value))+i));
        }
	free(point->value);
      }
    }
    break;

  case IEEE1888_DATATYPE_POINTSET:
    {
      ieee1888_pointSet* pointSet=(ieee1888_pointSet*)obj;
      if(pointSet->id!=NULL){	free(pointSet->id);	}
      if(pointSet->pointSet!=NULL){
        for(i=0;i<pointSet->n_pointSet;i++){
          ieee1888_destroy_objects((ieee1888_object*)(((ieee1888_pointSet*)(pointSet->pointSet))+i));
        }
	free(pointSet->pointSet);
      }
      if(pointSet->point!=NULL){
        for(i=0;i<pointSet->n_point;i++){
          ieee1888_destroy_objects((ieee1888_object*)(((ieee1888_point*)(pointSet->point))+i));
        }
	free(pointSet->point);
      }
    }
    break;


  case IEEE1888_DATATYPE_BODY:
    {
      ieee1888_body* body=(ieee1888_body*)obj;
      if(body->pointSet!=NULL){
        for(i=0;i<body->n_pointSet;i++){
          ieee1888_destroy_objects((ieee1888_object*)(((ieee1888_pointSet*)(body->pointSet))+i));
        }
	free(body->pointSet);
      }
      if(body->point!=NULL){
        for(i=0;i<body->n_point;i++){
          ieee1888_destroy_objects((ieee1888_object*)(((ieee1888_point*)(body->point))+i));
        }
	free(body->point);
      }
    }
    break;

  case IEEE1888_DATATYPE_TRANSPORT:
    {
      ieee1888_transport* transport=(ieee1888_transport*)obj;
      if(transport->header!=NULL){
        ieee1888_destroy_objects((ieee1888_object*)transport->header);
	free(transport->header);
      }
      if(transport->body!=NULL){
        ieee1888_destroy_objects((ieee1888_object*)transport->body);
	free(transport->body);
      }
    }
    break;

  }
}

/*
int main(int argc, char** argv){

  long l;

//   for(l=0;l<10000000;l++){
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

  ieee1888_object* clone=ieee1888_clone_objects((ieee1888_object*)transport,1);

  ieee1888_dump_objects((ieee1888_object*)clone);
  if(transport!=NULL){
    ieee1888_destroy_objects((ieee1888_object*)transport);
    free(transport);
  }

  // }

  return 0;
}

*/
