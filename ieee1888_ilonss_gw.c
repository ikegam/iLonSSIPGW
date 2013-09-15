/*
 * IEEE1888 - iLon/IP GW
 *
 * author: Hideya Ochiai
 * create: 2012-10-03
 * update: 2012-12-06  from Dallas to Tokyo
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "ilonss.h"
#include "ieee1888.h"
#include "ieee1888_datapool.h"

#define IEEE1888_ILONSS_POINTID_LEN     256
#define IEEE1888_ILONSS_VALUE_LEN      1024
#define IEEE1888_ILONSS_TIME_LEN         32
#define IEEE1888_ILONSS_POINT_COUNT    1024
#define IEEE1888_ILONSS_HOSTNAME_LEN     64

#define IEEE1888_ILONSS_BULK_SESSION_TIMEOUT 15

// access methods and access permission
#define IEEE1888_ILONSS_ACCESS_NONE      0
#define IEEE1888_ILONSS_ACCESS_READ      1
#define IEEE1888_ILONSS_ACCESS_WRITE     2
#define IEEE1888_ILONSS_ACCESS_READWRITE 3

#define IEEE1888_ILONSS_OK    0
#define IEEE1888_ILONSS_ERROR 1

// log level
#define IEEE1888_ILONSS_LOGLEVEL_DEBUG  1
#define IEEE1888_ILONSS_LOGLEVEL_INFO   2
#define IEEE1888_ILONSS_LOGLEVEL_WARN   3
#define IEEE1888_ILONSS_LOGLEVEL_ERROR  4

struct ilonssGW_baseConfig {
  char point_id[IEEE1888_ILONSS_POINTID_LEN];
  char host[IEEE1888_ILONSS_HOSTNAME_LEN];
  unsigned short port;
  char object_id[1024];
  char data_type[1024];
  int priority;
  uint8_t permission;
  int8_t exp;
  time_t status_time;
  char status_value[IEEE1888_ILONSS_VALUE_LEN];
};

struct ilonssGW_baseConfig m_config[IEEE1888_ILONSS_POINT_COUNT];
int n_m_config=0;

pthread_t __ilonssGW_writeClient_thread;
void* ilonssGW_writeClient_thread(void* args);

pthread_t __ilonssGW_fetchClient_thread;
void* ilonssGW_fetchClient_thread(void* args);

char m_writeServer_ids[IEEE1888_ILONSS_POINT_COUNT][IEEE1888_ILONSS_POINTID_LEN];
char m_fetchServer_ids[IEEE1888_ILONSS_POINT_COUNT][IEEE1888_ILONSS_POINTID_LEN];
char m_writeClient_ids[IEEE1888_ILONSS_POINT_COUNT][IEEE1888_ILONSS_POINTID_LEN];
char m_fetchClient_ids[IEEE1888_ILONSS_POINT_COUNT][IEEE1888_ILONSS_POINTID_LEN];

int n_m_writeServer_ids=0;
int n_m_fetchServer_ids=0;
int n_m_writeClient_ids=0;
int n_m_fetchClient_ids=0;

#define IEEE1888_SERVER_URL_LEN 256

char m_writeClient_ieee1888_server_url[IEEE1888_SERVER_URL_LEN];
int m_writeClient_trigger_frequency;
int m_writeClient_trigger_offset;

char m_fetchClient_ieee1888_server_url[IEEE1888_SERVER_URL_LEN];
int m_fetchClient_trigger_frequency;
int m_fetchClient_trigger_offset;

pthread_t __ilonssGW_printStatus_thread;
void* ilonssGW_printStatus_thread(void* args);
char m_printStatus_filepath[256];

int m_datapool_timespan;

void ilonssGW_log(const char* logMessage, int logLevel);


ieee1888_error* ilonssGW_pointsTest(char ids[][IEEE1888_ILONSS_POINTID_LEN], uint8_t access[], int n_point){

  int i,j;
  for(i=0;i<n_point;i++){
    for(j=0;j<n_m_config;j++){
      if(strcmp(ids[i],m_config[j].point_id)==0){
	if((access[i]&m_config[j].permission)!=access[i]){
	  char buf[1024];
	  if(access[i]==IEEE1888_ILONSS_ACCESS_READ){
            sprintf(buf,"Read access to %s is prohibitted.",ids[i]);
	  }else if(access[i]==IEEE1888_ILONSS_ACCESS_WRITE){
            sprintf(buf,"Write access to %s is prohibitted.",ids[i]);
	  }else{
            sprintf(buf,"Unknown access to %s is prohibitted.",ids[i]);
	  }
	  return ieee1888_mk_error_forbidden(buf);
	}
        break;
      }
    }
    if(j==n_m_config){
      char buf[1024];
      sprintf(buf,"%s",ids[i]);
      return ieee1888_mk_error_point_not_found(buf);
    }
  }
  return NULL;
}

int ilonssGW_findConfig(char id[], struct ilonssGW_baseConfig** pconfig){
  
  int j;
  for(j=0;j<n_m_config;j++){
    if(strcmp(id,m_config[j].point_id)==0){
      *pconfig=&m_config[j];
      return IEEE1888_ILONSS_OK;
    }
  }
  return IEEE1888_ILONSS_ERROR;
}

int ilonssGW_bacnetRead(char ids[][IEEE1888_ILONSS_POINTID_LEN], time_t times[], char values[][IEEE1888_ILONSS_VALUE_LEN], int n_point){

  int i;
  struct ilonssGW_baseConfig* config;
  time_t start=time(NULL);

  char black_host[8][IEEE1888_ILONSS_HOSTNAME_LEN];
  int n_black_host=0;

  for(i=0;i<n_point;i++){
    if(ilonssGW_findConfig(ids[i],&config)==IEEE1888_ILONSS_OK){

      time_t now=time(NULL);
      if(now<start || now>start+IEEE1888_ILONSS_BULK_SESSION_TIMEOUT){
        ilonssGW_log("iLon bulk session(read) timedout\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
        return IEEE1888_ILONSS_ERROR;
      }

      int k=-1;
      for(k=0;k<n_black_host;k++){
        if(strcmp(black_host[k],config->host)==0){
          break;
        }
      }
      if(k<n_black_host){
        continue;
      }

      // raw read: invoke readProperty
      struct ilon_data bdata;
      if(ILONSS_OK == readProperty(config->host,config->port,
            config->object_id,config->data_type ,
            &bdata) ){

        char value[IEEE1888_ILONSS_VALUE_LEN];
        memset(value,0,sizeof(value));

        time_t record_time=time(NULL);
        // copy value to the area of caller
        strncpy(values[i],bdata.value,IEEE1888_ILONSS_VALUE_LEN);
        times[i]=record_time;

        // load into the status buffer
        config->status_time=record_time;
        strncpy(config->status_value,bdata.value,IEEE1888_ILONSS_VALUE_LEN);

      }else{
        char logbuf[1000];
        sprintf(logbuf,"Failed to get data of %s from iLonSS\n",config->point_id);
        ilonssGW_log(logbuf,IEEE1888_ILONSS_LOGLEVEL_WARN);

        values[i][0]='\0';
        times[i]=0;

        config->status_time=0;
        config->status_value[0]='\0';

        if(n_black_host<8){
          strcpy(black_host[n_black_host],config->host);
          n_black_host++;
        }
        // return IEEE1888_ILONSS_ERROR; // error occured during bacnet.readProperty 
      }
    }
  }
  return IEEE1888_ILONSS_OK;
}

int ilonssGW_bacnetWrite(char ids[][IEEE1888_ILONSS_POINTID_LEN], char values[][IEEE1888_ILONSS_VALUE_LEN], int n_point){

  int i;
  struct ilonssGW_baseConfig *config;
  
  // pre-processing -- prepare binary objects to write (if error found, abort the mission.)
  struct ilon_data bdata[n_point]; 
 
  for(i=0;i<n_point;i++){
    if(ilonssGW_findConfig(ids[i],&config)==IEEE1888_ILONSS_OK){

      // parse the value and generate binary format according to the config.type and config.exp
      char value_str[IEEE1888_ILONSS_VALUE_LEN];
      memset(value_str,0,sizeof(value_str));
      strcpy(value_str,values[i]);

      // load into the status buffer
      config->status_time=time(NULL);
      strncpy(config->status_value,value_str,IEEE1888_ILONSS_VALUE_LEN);
    }
  }
  
  time_t start=time(NULL);
  
  char black_host[8][IEEE1888_ILONSS_HOSTNAME_LEN];
  int n_black_host=0;
  
  // write processing
  for(i=0;i<n_point;i++){
    if(ilonssGW_findConfig(ids[i],&config)==IEEE1888_ILONSS_OK){
      
      time_t now=time(NULL);
      if(now<start || now>start+IEEE1888_ILONSS_BULK_SESSION_TIMEOUT){
        ilonssGW_log("iLon bulk session(write) timedout\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
        return IEEE1888_ILONSS_ERROR;
      }
      
      int k=-1;
      for(k=0;k<n_black_host;k++){
        if(strcmp(black_host[k],config->host)==0){
          break;
        }
      }
      if(k<n_black_host){
        continue;
      }

      strcpy(bdata[i].value, config->status_value);
      strcpy(bdata[i].type, config->data_type);
      bdata[i].priority = config->priority;

      if(ILONSS_OK == writeProperty(config->host,config->port,
                  	   config->object_id,
                           &bdata[i]) ){
        // SUCCESS
      }else{
        char logbuf[1000];
        sprintf(logbuf,"Failed to set the data of %s into iLon\n",config->point_id);
        ilonssGW_log(logbuf,IEEE1888_ILONSS_LOGLEVEL_ERROR);

        if(n_black_host<8){
          strcpy(black_host[n_black_host],config->host);
          n_black_host++;
        }
        // return IEEE1888_ILONSS_ERROR;
      }
    }else{
      return IEEE1888_ILONSS_ERROR;
    }
  }
  if(n_black_host>0){
    return IEEE1888_ILONSS_ERROR;
  }
  return IEEE1888_ILONSS_OK;
}


ieee1888_error* ilonssGW_ieee1888read(ieee1888_point point[], int n_point, time_t timeAs){


  ilonssGW_log("ilonssGW_ieee1888read(begin)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);

  uint8_t access[n_point];
  memset(access,IEEE1888_ILONSS_ACCESS_READ,sizeof(access));

  int i;
  char ids[n_point][IEEE1888_ILONSS_POINTID_LEN];
  for(i=0;i<n_point;i++){
    if(strlen(point[i].id)>=IEEE1888_ILONSS_POINTID_LEN){
      ilonssGW_log("TOO LONG POINT ID\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
      ilonssGW_log("ilonssGW_ieee1888read(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
      return ieee1888_mk_error_server_error("TOO LONG POINT ID");
    }
    strncpy(ids[i],point[i].id,IEEE1888_ILONSS_POINTID_LEN);
  }

  ieee1888_error* myerr=ilonssGW_pointsTest(ids,access,n_point);
  if(myerr!=NULL){
    return myerr;
  }

  char values[n_point][IEEE1888_ILONSS_VALUE_LEN];
  time_t times[n_point];
  if(ilonssGW_bacnetRead(ids,times,values,n_point)==IEEE1888_ILONSS_OK){
    for(i=0;i<n_point;i++){
      if(times[i]!=0){
        ieee1888_value* v=ieee1888_mk_value();
        v->time=ieee1888_mk_time(timeAs);
        v->content=ieee1888_mk_string(values[i]);
        point[i].value=v;
        point[i].n_value=1;
      }
    }
    ilonssGW_log("ilonssGW_ieee1888read(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
    return NULL;
  }else{
    ilonssGW_log("ilonssGW bacnetRead failed\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
    ilonssGW_log("ilonssGW_ieee1888read(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
    return ieee1888_mk_error_server_error("ilonssGW bacnetRead failed");
  }
}

ieee1888_error* ilonssGW_ieee1888write(ieee1888_point point[], int n_point){

  ilonssGW_log("ilonssGW_ieee1888write(begin)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);

  uint8_t access[n_point];
  memset(access,IEEE1888_ILONSS_ACCESS_WRITE,sizeof(access));

  int i;
  char ids[n_point][IEEE1888_ILONSS_POINTID_LEN];
  char values[n_point][IEEE1888_ILONSS_VALUE_LEN];
  for(i=0;i<n_point;i++){
    if(strlen(point[i].id)>=IEEE1888_ILONSS_POINTID_LEN){
      ilonssGW_log("TOO LONG POINT ID\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
      ilonssGW_log("ilonssGW_ieee1888write(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
      return ieee1888_mk_error_server_error("TOO LONG POINT ID");
    }
    strncpy(ids[i],point[i].id,IEEE1888_ILONSS_POINTID_LEN);

    if(point[i].n_value>0 && point[i].value!=NULL){
      ieee1888_value* v=&(point[i].value[point[i].n_value-1]);
      if(strlen(v->content)>=IEEE1888_ILONSS_VALUE_LEN){
        ilonssGW_log("TOO LONG VALUE\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
        ilonssGW_log("ilonssGW_ieee1888write(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
        return ieee1888_mk_error_server_error("TOO LONG VALUE");
      }
      strncpy(values[i],v->content,IEEE1888_ILONSS_VALUE_LEN);
    }else{
      // nothing to do for index i
      values[i][0]='\0';
    }
  }
  
  // point id schema test
  ieee1888_error* myerr=ilonssGW_pointsTest(ids,access,n_point);
  if(myerr!=NULL){
    return myerr;
  }

  // shrink the ids
  int k_point=0;
  for(i=0;i<n_point;i++){
    if(values[i][0]=='\0'){
      continue;
    }
    if(k_point<i){
      strcpy(ids[k_point],ids[i]);
      strcpy(values[k_point],values[i]);
    }
    k_point++;
  }
  
  if(ilonssGW_bacnetWrite(ids,values,k_point)==IEEE1888_ILONSS_OK){
    ilonssGW_log("ilonssGW_ieee1888write(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
    return NULL;
  }else{
    ilonssGW_log("ilonssGW bacnetWrite failed\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
    ilonssGW_log("ilonssGW_ieee1888write(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
    return ieee1888_mk_error_server_error("ilonssGW bacnetWrite failed");
  }
}



/*
 * IEEE1888 service handlers
 *
 *  -- ilonssGW_ieee1888_server_query
 *       returns the data by reading from bacnet (by calling ilonssGW_ieee1888read method)
 *
 *  -- ilonssGW_ieee1888_server_data
 *       pushes data by writing to bacnet (by calling ilonssGW_ieee1888write method)
 *
 *  -- ilonssGW_ieee1888_server_data_parse_request
 *       parses the data request (prepare for committment)
 *
 *  -- ilonssGW_ieee1888_server_data_commit_request
 *       executes the data request (prepared for committment)
 *         by calling ilonssGW_ieee1888write method
 */
ieee1888_transport* ilonssGW_ieee1888_server_query(ieee1888_transport* request, char** args){

  ilonssGW_log("ilonssGW_ieee1888_server_query(begin)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);

  ieee1888_transport* response=(ieee1888_transport*)ieee1888_clone_objects((ieee1888_object*)request,1);
  
  if(response->body!=NULL){
    ieee1888_destroy_objects((ieee1888_object*)response->body);
    free(response->body);
    response->body=NULL;
  }

  ieee1888_header* header=response->header;
  if(header==NULL){
    response->header=ieee1888_mk_header();
    response->header->error=ieee1888_mk_error_invalid_request("No header in the request.");
    ilonssGW_log("INVALID_REQUEST (No header in the request)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
    ilonssGW_log("ilonssGW_ieee1888_server_query(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
    return response;
  }
  if(header->OK!=NULL){
    response->header->error=ieee1888_mk_error_invalid_request("Invalid OK in the header.");
    ilonssGW_log("INVALID_REQUEST (Invalid OK in the header)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
    ilonssGW_log("ilonssGW_ieee1888_server_query(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
    return response;
  }
  if(header->error!=NULL){
    response->header->error=ieee1888_mk_error_invalid_request("Invalid error in the header.");
    ilonssGW_log("INVALID_REQUEST (Invalid error in the header)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
    ilonssGW_log("ilonssGW_ieee1888_server_query(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
    return response;
  }

  ieee1888_query* query=header->query;
  if(header->query==NULL){
    response->header->error=ieee1888_mk_error_invalid_request("No query in the header.");
    ilonssGW_log("INVALID_REQUEST (No query in the header)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
    ilonssGW_log("ilonssGW_ieee1888_server_query(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
    return response;
  }

  ieee1888_error* error=NULL;
  if(strcmp(query->type,"storage")==0){

    if(query->callbackData!=NULL){
      // Do nothing (just ignore it)
    }
    if(query->callbackControl!=NULL){
      // Do nothing (just ignore it)
    }

    // Parse the keys in detail
    ieee1888_key* keys=query->key;
    int n_keys=query->n_key;

    ieee1888_point* points=NULL;
    int n_points=0;
    if(n_keys>0){
      points=ieee1888_mk_point_array(n_keys);
      n_points=n_keys;
    }

    int i;
    for(i=0;i<n_keys;i++){
      ieee1888_key* key=&keys[i];
      if(key->id==NULL){
        // error -- invalid id
	error=ieee1888_mk_error_invalid_request("ID is missing in the query key");
        ilonssGW_log("INVALID_REQUEST (ID is missing in the query key)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
        break;

      }else if(key->attrName==NULL){
        // error -- invalid attrName
	error=ieee1888_mk_error_invalid_request("attrName is missing in the query key");
        ilonssGW_log("INVALID_REQUEST (attrName is missing in the query key)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
        break;

      }else if(strcmp(key->attrName,"time")!=0){
        // error -- unsupported attrName
	error=ieee1888_mk_error_query_not_supported("attrName other than \"time\" are not supported.");
        ilonssGW_log("QUERY_NOT_SUPPORTED (attrName other than \"time\" are not supported.)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
        break;

      }else if(key->eq!=NULL){
        // error -- not supported 
	error=ieee1888_mk_error_query_not_supported("eq in the query key is not supported.");
        ilonssGW_log("QUERY_NOT_SUPPORTED (eq in the query key is not supported.)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
        break;

      }else if(key->neq!=NULL){
        // error -- not supported 
	error=ieee1888_mk_error_query_not_supported("neq in the query key is not supported.");
        ilonssGW_log("QUERY_NOT_SUPPORTED (neq in the query key is not supported.)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
        break;

      }else if(key->lt!=NULL){
        // error -- not supported 
	error=ieee1888_mk_error_query_not_supported("lt in the query key is not supported.");
        ilonssGW_log("QUERY_NOT_SUPPORTED (lt in the query key is not supported.)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
        break;

      }else if(key->gt!=NULL){
        // error -- not supported 
	error=ieee1888_mk_error_query_not_supported("gt in the query key is not supported.");
        ilonssGW_log("QUERY_NOT_SUPPORTED (gt in the query key is not supported.)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
        break;

      }else if(key->lteq!=NULL){
        // error -- not supported 
	error=ieee1888_mk_error_query_not_supported("lteq in the query key is not supported.");
        ilonssGW_log("QUERY_NOT_SUPPORTED (lteq in the query key is not supported.)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
        break;

      }else if(key->gteq!=NULL){
        // error -- not supported 
	error=ieee1888_mk_error_query_not_supported("gteq in the query key is not supported.");
        ilonssGW_log("QUERY_NOT_SUPPORTED (gteq in the query key is not supported.)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
        break;

      }else if(key->trap!=NULL){
        // error -- not supported 
	error=ieee1888_mk_error_query_not_supported("trap in the query key is not supported.");
        ilonssGW_log("QUERY_NOT_SUPPORTED (trap in the query key is not supported.)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
        break;

      }else if(key->select!=NULL && strcmp(key->select,"minimum")!=0 && strcmp(key->select,"maximum")!=0){
        // error -- invalid select
	error=ieee1888_mk_error_invalid_request("Invalid select in the query key.");
        ilonssGW_log("INVALID_REQUEST (Invalid select in the query key.)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
        break;

      }else{

        // fetchServer ID validity check
	int j;
	for(j=0;j<n_m_fetchServer_ids;j++){
	  if(strcmp(key->id,m_fetchServer_ids[j])==0){
	    points[i].id=ieee1888_mk_uri(key->id);
            break;
	  }
	}
	if(j==n_m_fetchServer_ids){
	  char sbuf[IEEE1888_ILONSS_POINTID_LEN+25];
	  sprintf(sbuf,"Not allowed for fetch %s",key->id);
	  error=ieee1888_mk_error_forbidden(sbuf);
	  break;
	}
      }
    }

    if(error==NULL && n_points>0){
      error=ilonssGW_ieee1888read(points,n_points,time(NULL));
    }

    if(error==NULL){
      // if no error (return OK)
      response->header->OK=ieee1888_mk_OK();
      response->body=ieee1888_mk_body();
      response->body->point=points;
      response->body->n_point=n_points;
    }else{
      // if error exists (return the error without body)
      response->header->error=error;
      ieee1888_body* body=ieee1888_mk_body();
      body->point=points;
      body->n_point=n_points;
      ieee1888_destroy_objects((ieee1888_object*)body);
      free(body);
    }

  }else if(strcmp(query->type,"stream")==0){
    // not supported
    error=ieee1888_mk_error_query_not_supported("type=\"stream\" in the query is not supported.");
    ilonssGW_log("QUERY_NOT_SUPPORTED (type=\"stream\" in the query is not supported.\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
    response->header->error=error;

  }else{
    // error (invalid request)
    error=ieee1888_mk_error_invalid_request("Invalid query type.");
    ilonssGW_log("INVALID_REQUEST (Invalid query type.\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
    response->header->error=error;
  }
  ilonssGW_log("ilonssGW_ieee1888_server_query(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
  return response;
}


ieee1888_error* ilonssGW_ieee1888_server_data_parse_request(ieee1888_pointSet* pointSet, int n_pointSet, ieee1888_point* point, int n_point, ieee1888_point* w_points, int* n_w_points){
  
  // fprintf(stdout,"ilonssGW_ieee1888_server_data_parse_request(begin)\n");

  int i,j;
  for(i=0;i<n_pointSet;i++){
    ieee1888_error* error=ilonssGW_ieee1888_server_data_parse_request(pointSet[i].pointSet, pointSet[i].n_pointSet, pointSet[i].point, pointSet[i].n_point, w_points, n_w_points);
    if(error!=NULL){
      return error;
    }
  }

  for(i=0;i<n_point;i++){
    for(j=0;j<n_m_writeServer_ids;j++){
      if(strcmp(point[i].id,m_writeServer_ids[j])==0){
        // clone the point and the values
	int n=*n_w_points;
	w_points[n].id=ieee1888_mk_uri(point[i].id);
	w_points[n].value=(ieee1888_value*)ieee1888_clone_objects((ieee1888_object*)point[i].value,point[i].n_value);
        w_points[n].n_value=point[i].n_value;
	++*n_w_points;
	break;
      }
    }
    if(j==n_m_writeServer_ids){
      // Error
      char ssbuf[128];
      char sbuf[200];
      strncpy(ssbuf,point[i].id,127);
      sprintf(sbuf,"No WRITE permission for the point(%s)",ssbuf);
      //fprintf(stdout,"ERROR: FORBIDDEN (%s)\n",sbuf);
      //fprintf(stdout,"ilonssGW_ieee1888_server_data_parse_request(end)\n");
      return ieee1888_mk_error_forbidden(sbuf);
    }
  }
  // fprintf(stdout,"ilonssGW_ieee1888_server_data_parse_request(end)\n");
  return NULL;
}

ieee1888_transport* ilonssGW_ieee1888_server_data(ieee1888_transport* request,char** args){
  
  ilonssGW_log("ilonssGW_ieee1888_server_data(begin)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);

  ieee1888_transport* response=ieee1888_mk_transport();

  // check the validity of body 
  ieee1888_body* body=request->body;
  if(body==NULL){
    response->header=ieee1888_mk_header();
    response->header->error=ieee1888_mk_error_invalid_request("No body in the request.");
    ilonssGW_log("INVALID_REQUEST (No body in the request)\n",IEEE1888_ILONSS_LOGLEVEL_WARN);
    ilonssGW_log("ilonssGW_ieee1888_server_data(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
    return response;
  }

  // Data buffer between parsing and committing the request
  ieee1888_point* w_points=ieee1888_mk_point_array(IEEE1888_ILONSS_POINT_COUNT);
  int n_w_points=0;

  // parse the "data" request (with preparing committing them), and returns error (only if failed).
  ieee1888_error* error=ilonssGW_ieee1888_server_data_parse_request(body->pointSet,body->n_pointSet,body->point,body->n_point,w_points,&n_w_points);

  if(error!=NULL){
     response->header=ieee1888_mk_header();
     response->header->error=error;

     int d=0;
     for(d=0;d<IEEE1888_ILONSS_POINT_COUNT;d++){
       ieee1888_destroy_objects((ieee1888_object*)(w_points+d));
     }
     free(w_points);
     char logbuf[2000];
     sprintf(logbuf,"error replied: type=%s message=%s \n",error->type,error->content);
     ilonssGW_log(logbuf,IEEE1888_ILONSS_LOGLEVEL_WARN);

     ilonssGW_log("ilonssGW_ieee1888_server_data(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
     return response;
  }
        
  // writeServer ID validity check
  int i,j;
  for(i=0;i<n_w_points;i++){
    for(j=0;j<n_m_writeServer_ids;j++){
      if(strcmp(w_points[i].id,m_writeServer_ids[j])==0){
        break;
      }
    }
    if(j==n_m_writeServer_ids){
      char sbuf[IEEE1888_ILONSS_POINTID_LEN*2];
      sprintf(sbuf,"Not allowed for WRITE %s",w_points[i].id);
      error=ieee1888_mk_error_forbidden(sbuf);
      break;
    }
    if(error!=NULL){
     response->header=ieee1888_mk_header();
     response->header->error=error;

     int d=0;
     for(d=0;d<IEEE1888_ILONSS_POINT_COUNT;d++){
       ieee1888_destroy_objects((ieee1888_object*)(w_points+d));
     }
     free(w_points);

     char logbuf[2000];
     sprintf(logbuf,"error replied: type=%s message=%s \n",error->type,error->content);
     ilonssGW_log(logbuf,IEEE1888_ILONSS_LOGLEVEL_WARN);

     ilonssGW_log("ilonssGW_ieee1888_server_data(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
     return response;
    }
  }

  // commit the "data" request, and returns error (only if failed).
  if(error==NULL){
    error=ilonssGW_ieee1888write(w_points,n_w_points);
  }

  if(error!=NULL){
     response->header=ieee1888_mk_header();
     response->header->error=error;

     int d=0;
     for(d=0;d<IEEE1888_ILONSS_POINT_COUNT;d++){
       ieee1888_destroy_objects((ieee1888_object*)(w_points+d));
     }
     free(w_points);
     
     char logbuf[2000];
     sprintf(logbuf,"error replied: type=%s message=%s \n",error->type,error->content);
     ilonssGW_log(logbuf,IEEE1888_ILONSS_LOGLEVEL_WARN);

     ilonssGW_log("ilonssGW_ieee1888_server_data(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
     return response;
  }

  // return OK, because succeeded.
  response->header=ieee1888_mk_header();
  response->header->OK=ieee1888_mk_OK();
     
  int d=0;
  for(d=0;d<IEEE1888_ILONSS_POINT_COUNT;d++){
    ieee1888_destroy_objects((ieee1888_object*)(w_points+d));
  }
  free(w_points);

  ilonssGW_log("ilonssGW_ieee1888_server_data(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
  return response;
}


/*
 *  Automatic Threads (for periodic execution of WRITE and FETCH)
 *
 *   -- ilonssGW_writeClient_thread
 *         periodically reads the bacnet, and WRITE into m_writeClient_ieee1888_server_url.
 *         (*) works only for pre-configured addresses by ilonssGW_writeClient_ids
 *       
 *   -- ilonssGW_fetchClient_thread
 *         periodically FETCH from m_fetchClient_ieee1888_server_url, and writes the bacnet.
 *         (*) works only for pre-configured addresses by ilonssGW_fetchClient_ids
 *
 */

void* ilonssGW_writeClient_thread(void* args){

  // if no points to work on --> return
  if(n_m_writeClient_ids==0){
    return NULL;
  }

  int i;
  sleep(10); // wait at the startup
  time_t last_upload=0;

  // for debug
  // int counter=0;

  while(1){

    // time check interval: 1 seconds
    sleep(1);

    time_t now=time(NULL);
    if((last_upload-m_writeClient_trigger_offset)/m_writeClient_trigger_frequency
           ==(now-m_writeClient_trigger_offset)/m_writeClient_trigger_frequency ){
      continue;
    }
    last_upload=now;

    // for debug
    // printf("count: %d\n",counter++);

    ieee1888_point* point=ieee1888_mk_point_array(n_m_writeClient_ids);
    int n_point=n_m_writeClient_ids;
    for(i=0;i<n_point;i++){
      point[i].id=ieee1888_mk_uri(m_writeClient_ids[i]);
    }

    // read from ilonss
    time_t time_to_present=(now/m_writeClient_trigger_frequency)*m_writeClient_trigger_frequency;
    if(ilonssGW_ieee1888read(point,n_point,time_to_present)==IEEE1888_ILONSS_OK){
        
      // send the data
      ilonssGW_log("ilonssGW_writeClient_thread_WRITE(begin)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
      ieee1888_transport* rq_transport=ieee1888_mk_transport();
      ieee1888_body* rq_body=ieee1888_mk_body();
      rq_transport->body=rq_body;
      rq_body->point=point;
      rq_body->n_point=n_point;

      ieee1888_transport* rs_transport=ieee1888_client_data(rq_transport,m_writeClient_ieee1888_server_url,NULL,NULL);
      //ieee1888_dump_objects((ieee1888_object*)rq_transport);
      if(rs_transport!=NULL && rs_transport->header!=NULL && rs_transport->header->OK!=NULL){
        ilonssGW_log("writeClient success\n",IEEE1888_ILONSS_LOGLEVEL_INFO);
      }else{
        // Failed --> DUMP INTO DATAPOOL for retry
        ieee1888_datapool_dump(rq_transport);

        ilonssGW_log("writeClient failure\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
        // fprintf(stdout," this ==> server \n");
        if(rq_transport!=NULL){
          // ieee1888_dump_objects((ieee1888_object*)rq_transport);
        }
        // fprintf(stdout," this <== server \n");
        if(rs_transport!=NULL){
          // ieee1888_dump_objects((ieee1888_object*)rs_transport);
	}
      }

      if(rq_transport!=NULL){
        ieee1888_destroy_objects((ieee1888_object*)rq_transport);
        free(rq_transport);
      }
      if(rs_transport!=NULL){
        ieee1888_destroy_objects((ieee1888_object*)rs_transport);
        free(rs_transport);
      }
      ilonssGW_log("ilonssGW_writeClient_thread_WRITE(end)\n",IEEE1888_ILONSS_LOGLEVEL_INFO);

    }else{
      ilonssGW_log("ilonssGW_writeClient_thread: no WRITE client operation because of the failure of ilonssGW_ieee1888read\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
    }

  }
}

void* ilonssGW_fetchClient_thread(void* args){
  
  // if no points to work on --> return
  if(n_m_fetchClient_ids==0){
    return NULL;
  }

  int i;
  sleep(10); // wait at the startup
  time_t last_download=0;

  // int counter=0; // for debug

  while(1){

    // time check interval: 1 seconds
    sleep(1);

    time_t now=time(NULL);
    if((last_download-m_fetchClient_trigger_offset)/m_fetchClient_trigger_frequency
           ==(now-m_fetchClient_trigger_offset)/m_fetchClient_trigger_frequency ){
      continue;
    }
    // for debug
    // printf("count:%d\n",counter++);
    
    last_download=now;

    ieee1888_key* keys=ieee1888_mk_key_array(n_m_fetchClient_ids);
    int n_keys=n_m_fetchClient_ids;
    for(i=0;i<n_keys;i++){
      keys[i].id=ieee1888_mk_uri(m_fetchClient_ids[i]);
      keys[i].attrName=ieee1888_mk_string("time");
      keys[i].select=ieee1888_mk_string("maximum");
    }

    // fetch the data (cursor function is not implemented)
    ilonssGW_log("ilonssGW_fetchClient_thread_FETCH(begin)\n", IEEE1888_ILONSS_LOGLEVEL_INFO);
    ieee1888_transport* rq_transport=ieee1888_mk_transport();
    ieee1888_header* rq_header=ieee1888_mk_header();
    ieee1888_query* rq_query=ieee1888_mk_query();
    rq_transport->header=rq_header;
    rq_header->query=rq_query;
    rq_query->type=ieee1888_mk_string("storage");
    rq_query->id=ieee1888_mk_new_uuid();
    rq_query->key=keys;
    rq_query->n_key=n_keys;

    ieee1888_transport* rs_transport=ieee1888_client_query(rq_transport,m_fetchClient_ieee1888_server_url,NULL,NULL);
    if(rs_transport!=NULL && rs_transport->header!=NULL && rs_transport->header->OK!=NULL && rs_transport->body!=NULL){
      ilonssGW_log("fetchClient success.\n", IEEE1888_ILONSS_LOGLEVEL_INFO);

      ieee1888_body* rs_body=rs_transport->body;
      ieee1888_point* rs_point=rs_body->point;
      int n_rs_point=rs_body->n_point;

      ieee1888_error* local_error=ilonssGW_ieee1888write(rs_point,n_rs_point);
      if(local_error!=NULL){
         ieee1888_destroy_objects((ieee1888_object*)local_error);
         free(local_error); 
      }

    }else{
      ilonssGW_log("fetchClient failure.\n", IEEE1888_ILONSS_LOGLEVEL_ERROR);
      // fprintf(stdout," this ==> server \n");
      if(rq_transport!=NULL){
        // ieee1888_dump_objects((ieee1888_object*)rq_transport);
      }
      // fprintf(stdout," this <== server \n");
      if(rs_transport!=NULL){
        // ieee1888_dump_objects((ieee1888_object*)rs_transport);
      }
    }

    if(rq_transport!=NULL){
      ieee1888_destroy_objects((ieee1888_object*)rq_transport);
      free(rq_transport);
    }
    if(rs_transport!=NULL){
      ieee1888_destroy_objects((ieee1888_object*)rs_transport);
      free(rs_transport);
    }
    ilonssGW_log("ilonssGW_fetchClient_thread_FETCH(end)\n", IEEE1888_ILONSS_LOGLEVEL_INFO);
  }
}

int ilonssGW_readConfig(const char* configPath){

  int i,k;

  ilonssGW_log("ilonssGW_readConfig(begin)\n", IEEE1888_ILONSS_LOGLEVEL_INFO);
    
  // initialize the memory space
  memset(m_config,0,sizeof(m_config));
  n_m_config=0;

  memset(m_writeServer_ids,0,sizeof(m_writeServer_ids));
  memset(m_fetchServer_ids,0,sizeof(m_fetchServer_ids));
  memset(m_writeClient_ids,0,sizeof(m_writeClient_ids));
  memset(m_fetchClient_ids,0,sizeof(m_fetchClient_ids));
  n_m_writeServer_ids=0;
  n_m_fetchServer_ids=0;
  n_m_writeClient_ids=0;
  n_m_fetchClient_ids=0;

  memset(m_writeClient_ieee1888_server_url,0,sizeof(m_writeClient_ieee1888_server_url));
  m_writeClient_trigger_frequency=0;
  m_writeClient_trigger_offset=0;

  memset(m_fetchClient_ieee1888_server_url,0,sizeof(m_fetchClient_ieee1888_server_url));
  m_fetchClient_trigger_frequency=0;
  m_fetchClient_trigger_offset=0;

  m_datapool_timespan=60;

  // open file
  FILE* fp=fopen(configPath,"r");
  if(fp==NULL){
    ilonssGW_log("Failed to read the configFile\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
    return IEEE1888_ILONSS_ERROR;
  }

  // `ead a line
  char line[2048];
  while(fgets(line,2048,fp)!=NULL){
    
    // eats spaces, tabs, etc..
    int len=strlen(line);
    for(i=0,k=0;i<len;i++){
      if(line[i]<=(char)0x1f){
        continue;
      }
      if(k<i){
        line[k]=line[i];
      }
      k++;
    }
    line[k]='\0';
  
    // parse 
    char* cp=line;
    char* columns[10];
    int n_columns;
    for(i=0;i<10;i++){
      if((columns[i]=strtok(cp,","))==NULL){
        break;
      }
      cp=NULL;
    }
    n_columns=i;
  
    // DEBUG code
    //int d;
    //printf("DEBUG: %s: ",cp);
    //for(d=0;d<n_columns;d++){
    //  printf("%s ",columns[d]);
    //}
    //printf("\n");
  
    // parse in detail and push into the memory space
    if(n_columns>=1){

      if(strcmp("PRINTSTATUS_PATH",columns[0])==0){
        strcpy(m_printStatus_filepath,columns[1]);

      }else if(strcmp("DATAPOOL_TIMESPAN_MIN",columns[0])==0){
        m_datapool_timespan=atoi(columns[1]);

      }else if(strcmp("IIF",columns[0])==0){
        if(n_columns!=9){
          ilonssGW_log("Too many columns found in IIF\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
          fclose(fp); return IEEE1888_ILONSS_ERROR;
	}

        struct ilonssGW_baseConfig conf;
        memset(&conf,0,sizeof(conf));
        if(strlen(columns[1])>=IEEE1888_ILONSS_POINTID_LEN){
          ilonssGW_log("The length of point id (in IIF) is too long\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
          fclose(fp); return IEEE1888_ILONSS_ERROR;
        }
        strcpy(conf.point_id,columns[1]);

        if(strlen(columns[2])>=IEEE1888_ILONSS_HOSTNAME_LEN){
          ilonssGW_log("The length of hostname (in IIF) is too long\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
          fclose(fp); return IEEE1888_ILONSS_ERROR;
        }
        strcpy(conf.host,columns[2]);

        if(strtol(columns[3],NULL,0)<1 || strtol(columns[3],NULL,0)>=65536){
          ilonssGW_log("Invalid port number is specified in IIF\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
          fclose(fp); return IEEE1888_ILONSS_ERROR;
        }
        conf.port=(uint16_t)strtol(columns[3],NULL,0);

        strcpy(conf.object_id, columns[4]);

        if(strlen(columns[5]) >= 1024){
          ilonssGW_log("Invalid property id is specified in IIF\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
          fclose(fp); return IEEE1888_ILONSS_ERROR;
        }
	
        strcpy(conf.data_type, columns[5]);

        if(strtol(columns[6],NULL,0)<1 || strtol(columns[6],NULL,0)>=65536){
          ilonssGW_log("Invalid priority is specified in IIF\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
          fclose(fp); return IEEE1888_ILONSS_ERROR;
        }
        conf.priority = (int)strtol(columns[6],NULL,0);

        if(strcmp("R",columns[7])==0){
          conf.permission=IEEE1888_ILONSS_ACCESS_READ;
        }else if(strcmp("W",columns[7])==0){
          conf.permission=IEEE1888_ILONSS_ACCESS_WRITE;
        }else if(strcmp("RW",columns[7])==0){
          conf.permission=IEEE1888_ILONSS_ACCESS_READ | IEEE1888_ILONSS_ACCESS_WRITE;
        }else if(strcmp("",columns[7])==0){
          conf.permission=IEEE1888_ILONSS_ACCESS_NONE;
        }else{
          ilonssGW_log("Unknown access permission is specifed in IIF (only R, W, RW are allowed)\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
          fclose(fp); return IEEE1888_ILONSS_ERROR;
        }
        int exp=atoi(columns[8]);
        if(exp<-5 || exp>5){
          ilonssGW_log("Invalid exponential specification  (it should be between -5 and 5)\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
          fclose(fp); return IEEE1888_ILONSS_ERROR;
        }
        conf.exp=exp;

        memcpy(&m_config[n_m_config++],&conf,sizeof(struct ilonssGW_baseConfig));

      }else if(strcmp("WSP",columns[0])==0){
        struct ilonssGW_baseConfig* conf;
        if(ilonssGW_findConfig(columns[1],&conf)==IEEE1888_ILONSS_OK){
          if(conf->permission&IEEE1888_ILONSS_ACCESS_WRITE){
            strcpy(m_writeServer_ids[n_m_writeServer_ids++],columns[1]);
          }else{
            char logbuf[1024];
            sprintf(logbuf,"Point id at %s,%s has no write permission at iLon interface (check the corresponding IIF section)\n",columns[0],columns[1]);
            ilonssGW_log(logbuf,IEEE1888_ILONSS_LOGLEVEL_ERROR);
            fclose(fp); return IEEE1888_ILONSS_ERROR;
          }
        }else{
          char logbuf[1024];
          sprintf(logbuf,"Point id at %s,%s is not defined by IIF before.\n",columns[0],columns[1]);
          ilonssGW_log(logbuf,IEEE1888_ILONSS_LOGLEVEL_ERROR);
          fclose(fp); return IEEE1888_ILONSS_ERROR;
        }

      }else if(strcmp("FSP",columns[0])==0){
        struct ilonssGW_baseConfig* conf;
        if(ilonssGW_findConfig(columns[1],&conf)==IEEE1888_ILONSS_OK){
          if(conf->permission&IEEE1888_ILONSS_ACCESS_READ){
            strcpy(m_fetchServer_ids[n_m_fetchServer_ids++],columns[1]);
          }else{
            char logbuf[1024];
            sprintf(logbuf,"Point id at %s,%s has no read permission at iLon interface (check the corresponding IIF section)\n",columns[0],columns[1]);
            ilonssGW_log(logbuf,IEEE1888_ILONSS_LOGLEVEL_ERROR);
            fclose(fp); return IEEE1888_ILONSS_ERROR;
          }
        }else{
          char logbuf[1024];
          sprintf(logbuf,"Point id at %s,%s is not defined by IIF before.\n",columns[0],columns[1]);
          ilonssGW_log(logbuf,IEEE1888_ILONSS_LOGLEVEL_ERROR);
          fclose(fp); return IEEE1888_ILONSS_ERROR;
        }

      }else if(strcmp("WCM",columns[0])==0){
        strcpy(m_writeClient_ieee1888_server_url,columns[1]);
        m_writeClient_trigger_frequency=atoi(columns[2]);
        m_writeClient_trigger_offset=atoi(columns[3]);

      }else if(strcmp("WCP",columns[0])==0){
        struct ilonssGW_baseConfig* conf;
        if(ilonssGW_findConfig(columns[1],&conf)==IEEE1888_ILONSS_OK){
          if(conf->permission&IEEE1888_ILONSS_ACCESS_READ){
            strcpy(m_writeClient_ids[n_m_writeClient_ids++],columns[1]);
          }else{
            char logbuf[1024];
            sprintf(logbuf,"Point id at %s,%s has no read permission at iLon interface (check the corresponding IIF section)\n",columns[0],columns[1]);
            ilonssGW_log(logbuf,IEEE1888_ILONSS_LOGLEVEL_ERROR);
            fclose(fp); return IEEE1888_ILONSS_ERROR;
          }
        }else{
          char logbuf[1024];
          sprintf(logbuf,"Point id at %s,%s is not defined by IIF before.\n",columns[0],columns[1]);
          ilonssGW_log(logbuf,IEEE1888_ILONSS_LOGLEVEL_ERROR);
          fclose(fp); return IEEE1888_ILONSS_ERROR;
        }

      }else if(strcmp("FCM",columns[0])==0){
        strcpy(m_fetchClient_ieee1888_server_url,columns[1]);
        m_fetchClient_trigger_frequency=atoi(columns[2]);
        m_fetchClient_trigger_offset=atoi(columns[3]);
    
      }else if(strcmp("FCP",columns[0])==0){
        struct ilonssGW_baseConfig* conf;
        if(ilonssGW_findConfig(columns[1],&conf)==IEEE1888_ILONSS_OK){
          if(conf->permission&IEEE1888_ILONSS_ACCESS_WRITE){
            strcpy(m_fetchClient_ids[n_m_fetchClient_ids++],columns[1]);
          }else{
            char logbuf[1024];
            sprintf(logbuf,"Point id at %s,%s has no write permission at iLon interface (check the corresponding IIF section)\n",columns[0],columns[1]);
            ilonssGW_log(logbuf,IEEE1888_ILONSS_LOGLEVEL_ERROR);
            fclose(fp); return IEEE1888_ILONSS_ERROR;
          }
        }else{
          char logbuf[1024];
          sprintf(logbuf,"Point id at %s,%s is not defined by IIF before.\n",columns[0],columns[1]);
          ilonssGW_log(logbuf,IEEE1888_ILONSS_LOGLEVEL_ERROR);
          fclose(fp); return IEEE1888_ILONSS_ERROR;
        }
      }

      // error check
      if(n_m_config>IEEE1888_ILONSS_POINT_COUNT){
        ilonssGW_log("ERROR: too many IIFs\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
        fclose(fp); return IEEE1888_ILONSS_ERROR;
      }
      if(n_m_writeServer_ids>IEEE1888_ILONSS_POINT_COUNT){
        ilonssGW_log("ERROR: too many writeServer ids\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
        fclose(fp); return IEEE1888_ILONSS_ERROR;
      }
      if(n_m_fetchServer_ids>IEEE1888_ILONSS_POINT_COUNT){
        ilonssGW_log("ERROR: too many fetchServer ids\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
        fclose(fp); return IEEE1888_ILONSS_ERROR;
      }
      if(n_m_writeClient_ids>IEEE1888_ILONSS_POINT_COUNT){
        ilonssGW_log("ERROR: too many writeClient ids\n",IEEE1888_ILONSS_LOGLEVEL_ERROR);
        fclose(fp); return IEEE1888_ILONSS_ERROR;
      }
      if(n_m_fetchClient_ids>IEEE1888_ILONSS_POINT_COUNT){
        ilonssGW_log("ERROR: too many fetchClient ids",IEEE1888_ILONSS_LOGLEVEL_ERROR);
        fclose(fp); return IEEE1888_ILONSS_ERROR;
      }
    
    }
  }

  // close the file
  fclose(fp);
  
  ilonssGW_log("ilonssGW_readConfig(end)\n", IEEE1888_ILONSS_LOGLEVEL_INFO);

  return IEEE1888_ILONSS_OK;
}

void ilonssGW_printStatus(FILE* fp){
  
  ilonssGW_log("ilonssGW_printStatus(begin)\n", IEEE1888_ILONSS_LOGLEVEL_INFO);
  
  fprintf(fp,"<html><header><title>IEEE1888 - iLon/IP GW Status Page</title>\n");
  fprintf(fp,"<style type=\"text/css\">\n");
  fprintf(fp,"#header  {BACKGROUND-COLOR:#99ffcc; TEXT-ALIGN:CENTER;} \n");
  fprintf(fp,"#oddrow  {BACKGROUND-COLOR:#f0f0ff; TEXT-ALIGN:LEFT;}   \n");
  fprintf(fp,"#evenrow {BACKGROUND-COLOR:#f8f8ff; TEXT-ALIGN:LEFT;} \n");
  fprintf(fp,"</style></header><body>\n");
  fprintf(fp,"<h1>IEEE1888 - iLon/IP GW Status Page</h1>\n");
  fprintf(fp,"<table border=\"2\">");
  fprintf(fp,"<tr id=\"header\">\n");
  fprintf(fp,"<td>IEEE1888 Point ID</td>\n");
  fprintf(fp,"<td>iLon/IP HostName</td>\n");
  fprintf(fp,"<td>UDP Port</td>\n");
  fprintf(fp,"<td>Object ID</td>\n");
  fprintf(fp,"<td>Type</td>\n");
  fprintf(fp,"<td>Permission</td>\n");
  fprintf(fp,"<td>Time</td>\n");
  fprintf(fp,"<td>Value</td>\n");
  fprintf(fp,"</tr>\n");

  fflush(fp);
 
  int i; 
  for(i=0;i<n_m_config;i++){
    struct ilonssGW_baseConfig* p=&m_config[i];
    char spermission[10];
    char stime[40];
    struct tm tm_time;
    char style[20];
    if(i%2==0){
      strcpy(style,"evenrow");
    }else{
      strcpy(style,"oddrow");
    }
    switch(p->permission){
    case IEEE1888_ILONSS_ACCESS_READ: strcpy(spermission,"R"); break;
    case IEEE1888_ILONSS_ACCESS_WRITE: strcpy(spermission,"W"); break;
    case IEEE1888_ILONSS_ACCESS_READ | IEEE1888_ILONSS_ACCESS_WRITE: 
       strcpy(spermission,"RW"); break;
    default: strcpy(spermission,"ERROR");
    }
    localtime_r(&(p->status_time),&tm_time);
    if(p->status_time!=0){
      strftime(stime,40,"%Y-%m-%d %H:%M:%S",&tm_time);
    }else{
      stime[0]=' ';
      stime[1]='\0';
    }

    fprintf(fp,"<tr id=\"%s\">\n",style);
    fprintf(fp,"<td>%s</td>\n",p->point_id);
    fprintf(fp,"<td>%s</td>\n",p->host);
    fprintf(fp,"<td>%5d</td>\n",p->port);
    fprintf(fp,"<td>%s</td>\n",p->object_id);
    fprintf(fp,"<td>%s</td>\n",p->data_type);
    fprintf(fp,"<td>%s</td>\n",spermission);
    fprintf(fp,"<td>%s</td>\n",stime);
    if(strlen(p->status_value)==0){
      fprintf(fp,"<td> </td>\n");
    }else{
      fprintf(fp,"<td>%s</td>\n",p->status_value);
    }
    fprintf(fp,"</tr>\n");
  }
  fprintf(fp,"</table></body></html>");
  
  ilonssGW_log("ilonssGW_printStatus(end)\n", IEEE1888_ILONSS_LOGLEVEL_INFO);
}

void* ilonssGW_printStatus_thread(void* args){
  while(strlen(m_printStatus_filepath)>0){
    FILE* fp=fopen(m_printStatus_filepath,"w");
    if(fp){
      ilonssGW_printStatus(fp);
      fclose(fp);
    }
    sleep(30);
  }
  return NULL;
}


void ilonssGW_printConfig(FILE* fp){

  int i;
  ilonssGW_log("ilonssGW_printConfig(begin)\n", IEEE1888_ILONSS_LOGLEVEL_INFO);

  fprintf(fp,"PRINTSTATUS_PATH,%s\n",m_printStatus_filepath);  
  fprintf(fp,"DATAPOOL_TIMESPAN_MIN,%d\n",m_datapool_timespan);  

  for(i=0;i<n_m_config;i++){
    struct ilonssGW_baseConfig* p=&m_config[i];
    char sdatatype[10];
    char spermission[10];
    switch(p->permission){
    case IEEE1888_ILONSS_ACCESS_READ: strcpy(spermission,"R"); break;
    case IEEE1888_ILONSS_ACCESS_WRITE: strcpy(spermission,"W"); break;
    case IEEE1888_ILONSS_ACCESS_READ | IEEE1888_ILONSS_ACCESS_WRITE: 
       strcpy(spermission,"RW"); break;
    default: strcpy(spermission,"ERROR");
    }

    fprintf(fp,"IIF,%s,%s,%d,%s,%s,%s,%d\n",p->point_id,p->host,p->port,p->object_id,sdatatype,spermission,p->exp);
  }
  
  for(i=0;i<n_m_writeServer_ids;i++){
    fprintf(fp,"WSP,%s\n",m_writeServer_ids[i]);
  }
  for(i=0;i<n_m_fetchServer_ids;i++){
    fprintf(fp,"FSP,%s\n",m_fetchServer_ids[i]);
  }
  fprintf(fp,"WCM,%s,%d,%d\n",m_writeClient_ieee1888_server_url,m_writeClient_trigger_frequency,m_writeClient_trigger_offset);

  for(i=0;i<n_m_writeClient_ids;i++){
    fprintf(fp,"WCP,%s\n",m_writeClient_ids[i]);
  }
  fprintf(fp,"FCM,%s,%d,%d\n",m_fetchClient_ieee1888_server_url,m_fetchClient_trigger_frequency,m_fetchClient_trigger_offset);
  for(i=0;i<n_m_fetchClient_ids;i++){
    fprintf(fp,"FCP,%s\n",m_fetchClient_ids[i]);
  }
  ilonssGW_log("ilonssGW_printConfig(end)\n", IEEE1888_ILONSS_LOGLEVEL_INFO);
}

/**
 * LogManager
 *
 */
char ilonssGW_logPath[256];
int ilonssGW_logLevel_Threshold;
pthread_mutex_t ilonssGW_log_mx;

void ilonssGW_log(const char* logMessage, int logLevel){
  if(logLevel>=ilonssGW_logLevel_Threshold){
    pthread_mutex_lock(&ilonssGW_log_mx);
    FILE* fp=fopen(ilonssGW_logPath,"a");
    if(fp!=NULL){

      switch(logLevel){
      case IEEE1888_ILONSS_LOGLEVEL_DEBUG: fprintf(fp,"[DEBUG] "); break;
      case IEEE1888_ILONSS_LOGLEVEL_INFO:  fprintf(fp,"[INFO]  "); break;
      case IEEE1888_ILONSS_LOGLEVEL_WARN:  fprintf(fp,"[WARN]  "); break;
      case IEEE1888_ILONSS_LOGLEVEL_ERROR: fprintf(fp,"[ERROR] "); break;
      default:                               fprintf(fp,"[-----] ");
      }
      fprintf(fp,"%s",logMessage);
      fclose(fp);
    }
    pthread_mutex_unlock(&ilonssGW_log_mx);
  }
}

/*
 * Initializer
 */ 
int ilonssGW_init(const char* configPath, const char* logPath, const char* dpPath){
  
  strncpy(ilonssGW_logPath,logPath,256);
  ilonssGW_logLevel_Threshold=IEEE1888_ILONSS_LOGLEVEL_INFO;

  ilonssGW_log("ilonssGW_init (begin) \n",IEEE1888_ILONSS_LOGLEVEL_INFO);
  
  int ret=ilonssGW_readConfig(configPath);
  if(ret!=IEEE1888_ILONSS_OK){
    return ret;
  }

  // ilonssGW_printConfig(stdout);
  
  if(n_m_writeClient_ids>0 && m_writeClient_trigger_frequency>0){
    ieee1888_datapool_init(dpPath,m_writeClient_ieee1888_server_url,m_datapool_timespan);
    pthread_create(&__ilonssGW_writeClient_thread,0,ilonssGW_writeClient_thread,0);
  }

  if(n_m_fetchClient_ids>0 && m_fetchClient_trigger_frequency>0){
    pthread_create(&__ilonssGW_fetchClient_thread,0,ilonssGW_fetchClient_thread,0);
  }

  // print status thread
  pthread_create(&__ilonssGW_printStatus_thread,0,ilonssGW_printStatus_thread,0);

  ilonssGW_log("ilonssGW_init (end) \n",IEEE1888_ILONSS_LOGLEVEL_INFO);
  return IEEE1888_ILONSS_OK;
}

void ilonssGW_printUsage(){
  printf("Usage: ieee1888_ilonss_gw -c CONFIG_PATH -l LOG_PATH -p DATAPOOL_PATH\n\n");
}

int main(int argc, char* argv[]){

  if(argc!=7){
    ilonssGW_printUsage();
    return 1;
  }

  if(   strcmp(argv[1],"-c")!=0
     || strcmp(argv[3],"-l")!=0
     || strcmp(argv[5],"-p")!=0){
    ilonssGW_printUsage();
    return 1;
  }

  if(ilonssGW_init(argv[2],argv[4],argv[6])==IEEE1888_ILONSS_OK){

    ieee1888_set_service_handlers(ilonssGW_ieee1888_server_query,ilonssGW_ieee1888_server_data);
    int ret=ieee1888_server_create(1888);
    return ret;
  }
   
  return IEEE1888_ILONSS_ERROR;
}
