/**
 * ieee1888_datapool.c
 *
 * author: Hideya Ochiai
 * create: 2012-12-06 from Dallas to Tokyo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "ieee1888.h"
#include "ieee1888_datapool.h"

#define IEEE1888_DATAPOOL_PATH_LEN 256
#define IEEE1888_DATAPOOL_IEEE1888_SERVER_URL_LEN 512

pthread_t __ieee1888_datapool_mgmt_thread;
void* ieee1888_datapool_mgmt_thread(void* args);
void ieee1888_datapool_dump(ieee1888_transport* transport);

/**
 * DataPool Manager
 *  -- public: ieee1888_datapool_init
 *        initializes the datapool management module
 *
 *  -- public: ieee1888_datapool_dump
 *        dumps the data of a WRITE request into DataPool
 *
 *  -- private: ieee1888_datapool_create_dataRQ_clearCache
 *        clears the cache of point data for generating a WRITE request (to do an retry)
 *
 *  -- private: ieee1888_datapool_create_dataRQ_lookupPoint
 *        looks up (or create) an entry of point data from the cache
 *
 *  -- private: ieee1888_datapool_create_dataRQ_push
 *        pushes an record of the DataPool(id,time,value) into the cache
 *
 *  -- private: ieee1888_datapool_try_dataRQ
 *        try WRITE request to ieee1888_datapool_IEEE1888_SERVER_URL using the cache
 *
 *  -- private: ieee1888_datapool_parse_CSV_line
 *        a CSV line parser for getting id, time, and value from DataPool
 *
 *  -- private: ieee1888_datapool_vacuum
 *        vacuums the DataPool (delete older data than 24 hours, or currupted data in the DataPool)
 *
 *  -- thread: ieee1888_datapool_mgmt_thread
 *        datapool manager that initiates vaccume, retry, time control.
 *
 */
char ieee1888_datapool_path[IEEE1888_DATAPOOL_PATH_LEN];
pthread_mutex_t ieee1888_datapool_mx;

void ieee1888_datapool_dump(ieee1888_transport* transport){

  // printf("ieee1888_datapool_dump (begin)\n");

  if(transport==NULL){
    return ;
  }
  if(transport->body==NULL){
    return ;
  }
  if(transport->body->point==NULL || transport->body->n_point==0){
    return ;
  }

  ieee1888_point* point=transport->body->point;
  int n_point=transport->body->n_point;

  pthread_mutex_lock(&ieee1888_datapool_mx); // -- begin datapool lock
  FILE* fp=fopen(ieee1888_datapool_path,"a");
  if(fp==NULL){
    pthread_mutex_unlock(&ieee1888_datapool_mx); // -- end datapool lock
    printf("ieee1888_datapool_dump: Error failed to open datapool file.\n");
    // printf("ieee1888_datapool_dump (end)\n");
    return ;
  }

  int i;
  for(i=0;i<n_point;i++){
    if(point[i].value==NULL || point[i].n_value==0){
      continue;
    }
    ieee1888_value* value=point[i].value;
    int n_value=point[i].n_value;

    int k;
    for(k=0;k<n_value;k++){
      fprintf(fp,"%s,%s,%s\n",point[i].id,value[k].time,value[k].content);
    }
  }
  // For mutual exclusion test
  // printf("dump wait begin... \n");
  // sleep(100);
  // printf("dump wait end... \n");
  fclose(fp);
  pthread_mutex_unlock(&ieee1888_datapool_mx); // -- end datapool lock

  // printf("ieee1888_datapool_dump (end)\n");
}

#define IEEE1888_DATAPOOL_CREATE_DATARQ_MAX_DATAPOINTS 1024
ieee1888_point* ieee1888_datapool_create_dataRQ_points[IEEE1888_DATAPOOL_CREATE_DATARQ_MAX_DATAPOINTS];
int ieee1888_datapool_create_dataRQ_n_points=0;

void ieee1888_datapool_create_dataRQ_clearCache(){

  int i;
  // printf("ieee1888_datapool_create_dataRQ_clearCache (begin)\n");
  for(i=0;i<ieee1888_datapool_create_dataRQ_n_points;i++){
    if(ieee1888_datapool_create_dataRQ_points[i]!=NULL){
      ieee1888_destroy_objects((ieee1888_object*)ieee1888_datapool_create_dataRQ_points[i]);
      free(ieee1888_datapool_create_dataRQ_points[i]);
      ieee1888_datapool_create_dataRQ_points[i]=NULL;
    }
  }
  ieee1888_datapool_create_dataRQ_n_points=0;
  // printf("ieee1888_datapool_create_dataRQ_clearCache (end)\n");
}

ieee1888_point* ieee1888_datapool_create_dataRQ_lookupPoint(const char* id){

  int i;
  //printf("ieee1888_datapool_create_dataRQ_lookupPoint (begin)\n");
  for(i=0;i<ieee1888_datapool_create_dataRQ_n_points;i++){
    if(ieee1888_datapool_create_dataRQ_points[i]->id==NULL){
      printf("FATAL error at ieee1888_datapool_create_dataRQ_lookupPoint: skip...\n");
      //printf("ieee1888_datapool_create_dataRQ_lookupPoint (end)\n");
      continue;
    }
    if(strcmp(ieee1888_datapool_create_dataRQ_points[i]->id,id)==0){
      //printf("ieee1888_datapool_create_dataRQ_lookupPoint (end)\n");
      return ieee1888_datapool_create_dataRQ_points[i];
    }
  }
  if(ieee1888_datapool_create_dataRQ_n_points<IEEE1888_DATAPOOL_CREATE_DATARQ_MAX_DATAPOINTS){
    ieee1888_datapool_create_dataRQ_points[ieee1888_datapool_create_dataRQ_n_points]=ieee1888_mk_point();
    ieee1888_datapool_create_dataRQ_points[ieee1888_datapool_create_dataRQ_n_points]->id=ieee1888_mk_uri(id);
    //printf("ieee1888_datapool_create_dataRQ_lookupPoint (end)\n");
    return ieee1888_datapool_create_dataRQ_points[ieee1888_datapool_create_dataRQ_n_points++];
  }
  printf("ieee1888_datapool_create_dataRQ_lookupPoint (error) -- some points(data) may have been lost.\n");
  //printf("ieee1888_datapool_create_dataRQ_lookupPoint (end)\n");
  return NULL;
}

void ieee1888_datapool_create_dataRQ_push(const char* id, const char* time, const char* content){

  // printf("ieee1888_datapool_create_dataRQ_push (begin)\n");
  ieee1888_point* point=ieee1888_datapool_create_dataRQ_lookupPoint(id);
  if(point==NULL){
    printf("ieee1888_datapool_create_dataRQ_push: FATAL error at ieee1888_datapool_create_dataRQ_push -- no point entry...probably entry size should be expanded.\n");
    //printf("ieee1888_datapool_create_dataRQ_push (end)\n");
    return ; // fatal error
  }

  if(point->value==NULL || point->n_value==0){
    ieee1888_value* value=ieee1888_mk_value();
    value->time=ieee1888_mk_string(time);
    value->content=ieee1888_mk_string(content);
    point->value=value;
    point->n_value=1;
  }else{
    ieee1888_value* add_value=ieee1888_mk_value();
    add_value->time=ieee1888_mk_string(time);
    add_value->content=ieee1888_mk_string(content);
    ieee1888_value* new_value=ieee1888_add_value_to_array(point->value,point->n_value,add_value);
    int n_new_value=point->n_value+1;

    free(point->value);
    free(add_value);

    point->value=new_value;
    point->n_value=n_new_value;
  }
  // printf("ieee1888_datapool_create_dataRQ_push (end)\n");
}


char ieee1888_datapool_IEEE1888_SERVER_URL[IEEE1888_DATAPOOL_IEEE1888_SERVER_URL_LEN];

#define IEEE1888_DATAPOOL_IEEE1888_SUCCESS         0
#define IEEE1888_DATAPOOL_IEEE1888_TRANSPORT_ERROR 1
#define IEEE1888_DATAPOOL_IEEE1888_SERVICE_ERROR   2  /* server returned <error ... >...</error> */
int ieee1888_datapool_try_dataRQ(){

  // printf("ieee1888_datapool_try_dataRQ (begin)\n");

  ieee1888_transport* rq_transport=ieee1888_mk_transport();
  ieee1888_body* rq_body=ieee1888_mk_body();
  ieee1888_point* rq_points=ieee1888_mk_point_array(ieee1888_datapool_create_dataRQ_n_points);
  rq_transport->body=rq_body;
  rq_body->point=rq_points;
  rq_body->n_point=ieee1888_datapool_create_dataRQ_n_points;

  int i;
  for(i=0;i<ieee1888_datapool_create_dataRQ_n_points;i++){
    rq_points[i].id=ieee1888_mk_uri(ieee1888_datapool_create_dataRQ_points[i]->id);
    rq_points[i].value=(ieee1888_value*)ieee1888_clone_objects(
                (ieee1888_object*)ieee1888_datapool_create_dataRQ_points[i]->value,
                ieee1888_datapool_create_dataRQ_points[i]->n_value);
    rq_points[i].n_value=ieee1888_datapool_create_dataRQ_points[i]->n_value;
  }

  // call to the server
  ieee1888_transport* rs_transport=ieee1888_client_data(rq_transport,ieee1888_datapool_IEEE1888_SERVER_URL,NULL,NULL);

  if(rs_transport!=NULL && rs_transport->header!=NULL && rs_transport->header->OK!=NULL){
    // printf("WRITE success(datapool).\n");
  }else{
    // printf("WRITE failed(datapool).\n");

    // fprintf(stdout,"WRITE failed(datapool).\n");
    // fprintf(stdout," this ==> server \n");
    if(rq_transport!=NULL){
      // ieee1888_dump_objects((ieee1888_object*)rq_transport);
    }
    // fprintf(stdout," this <== server \n");
    if(rs_transport!=NULL){
     //  ieee1888_dump_objects((ieee1888_object*)rs_transport);
    }
  }

  if(rq_transport!=NULL){
    ieee1888_destroy_objects((ieee1888_object*)rq_transport);
    free(rq_transport);
  }
  if(rs_transport!=NULL){
    ieee1888_destroy_objects((ieee1888_object*)rs_transport);
    free(rs_transport);
  }else{
    // printf("ieee1888_datapool_try_dataRQ (transport error -- try again)\n"); // this is not an error.
    // printf("ieee1888_datapool_try_dataRQ (end)\n");
    return IEEE1888_DATAPOOL_IEEE1888_TRANSPORT_ERROR;
  }
  // printf("ieee1888_datapool_try_dataRQ (end)\n");
  return IEEE1888_DATAPOOL_IEEE1888_SUCCESS;
}

int ieee1888_datapool_parse_CSV_line(const char* line, char* row_id, char* row_time, char* row_value){

  int i;
  int column=0;
  int row_id_index=0;
  int row_time_index=0;
  int row_value_index=0;
  int len=strlen(line);
  for(i=0;i<len;i++){
    if(line[i]=='\n'){ break; }
      if(line[i]==','){
        column++;
        continue;
      }
    switch(column){
    case 0:
      if(row_id_index<256-1){
        row_id[row_id_index++]=line[i];
      }else{
        printf("the lendth of id section is too long...\n");
        return 0;
      }
      break;

    case 1:
      if(row_time_index<50-1){
        row_time[row_time_index++]=line[i];
      }else{
        printf("the lendth of time section is too long...\n");
        return 0;
      }
      break;
    case 2:
      if(row_value_index<40-1){
        row_value[row_value_index++]=line[i];
      }else{
        printf("the lendth of value section is too long...\n");
        return 0;
      }
      break;
    default: i=len;
    }
  }
  row_id[row_id_index]='\0';
  row_time[row_time_index]='\0';
  row_value[row_value_index]='\0';
  if(column!=2){
    printf("Error while parsing a line of datapool CSV ...\n");
    return 0;
  }
  return 1;
}

int ieee1888_datapool_timespan_min=60;

void ieee1888_datapool_vacuum(){

  // printf("ieee1888_datapool_vacuum (begin)\n");

  char line[1024];
  char row_id[256];
  char row_time[50];
  char row_value[40];

  pthread_mutex_lock(&ieee1888_datapool_mx); // -- begin datapool lock

  // Prepare Reading 
  FILE* fp=fopen(ieee1888_datapool_path,"r");
  if(fp==NULL){
    // printf("ieee1888_datapool_vacuum (end)\n"); // this is not an error
    pthread_mutex_unlock(&ieee1888_datapool_mx); // -- end datapool lock
    return;
  }

  // Prepare buffer file 
  char buffer_path[200];
  sprintf(buffer_path,"%s.buf",ieee1888_datapool_path);
  FILE* fp_buffer=fopen(buffer_path,"w");
  if(fp_buffer==NULL){
    if(fp!=NULL){
      fclose(fp);
    }
    pthread_mutex_unlock(&ieee1888_datapool_mx); // -- end datapool lock
    printf("ieee1888_datapool_vacuum: buffer file open error \n");
    // printf("ieee1888_datapool_vacuum (end)\n");
    return;
  }
  while(fgets(line,1024,fp)!=NULL){
    if(ieee1888_datapool_parse_CSV_line(line,row_id,row_time,row_value)){
      // filters by simply just compares the string expression of timestamp at 1 day ago
      int filter_pass=0;
      char* past_border_time=ieee1888_mk_time(time(NULL)-60*ieee1888_datapool_timespan_min);
      if(strcmp(row_time,past_border_time)>0){
        filter_pass=1;
      }else{
        filter_pass=0;
      }
      free(past_border_time);
      if(filter_pass==0){
        continue;
      }
      // Save into the buffer
      fputs(line,fp_buffer);
    }
  }
  fclose(fp);
  fclose(fp_buffer);

  // for mutual exclusion test
  // printf("vacuum wait begin...\n");
  // sleep(60);
  // printf("vacuum wait end...\n");

  // copy back (begin)
  fp_buffer=fopen(buffer_path,"r");
  if(fp_buffer==NULL){
    pthread_mutex_unlock(&ieee1888_datapool_mx); // -- end datapool lock
    printf("Error while copying back from the buffer to the main datapool -- failed to open buffer file... (ieee1888_datapool_vaccum)\n");
    // printf("ieee1888_datapool_vacuum (end)\n");
    return ;
  }
  fp=fopen(ieee1888_datapool_path,"w");
  if(fp==NULL){
    printf("Error while copying back from the buffer to the main datapool -- failed to open datapool file... (ieee1888_datapool_vaccum)\n");
    if(fp_buffer!=NULL){
      fclose(fp_buffer);
    }
    pthread_mutex_unlock(&ieee1888_datapool_mx); // -- end datapool lock
    // printf("ieee1888_datapool_vacuum (end)\n");
    return ;
  }
  int file_empty=1;
  while(fgets(line,1024,fp_buffer)!=NULL){
    file_empty=0;
    fputs(line,fp);
  }
  fclose(fp);
  fclose(fp_buffer);
  remove(buffer_path);
  if(file_empty){
    remove(ieee1888_datapool_path);
  }
  // copy back (end)
  pthread_mutex_unlock(&ieee1888_datapool_mx); // -- end datapool lock
  // printf("ieee1888_datapool_vacuum (end)\n");
}

void* ieee1888_datapool_mgmt_thread(void *args){

  sleep(10);
  // printf("ieee1888_datapool_mgmt_thread (begin)\n");

  char line[1024];
  char row_id[256];
  char row_time[50];
  char row_value[40];
  int row_index=0;
  int row_limit=10;

  time_t last_vacuum=0;

  while(1){

    // Vacuum Management Process
    // printf("ieee1888_datapool_mgmt_thread_vacuum_mgmt (begin)\n");
    time_t now=time(NULL);
    if(now<last_vacuum || now>last_vacuum+600){
      ieee1888_datapool_vacuum();
      last_vacuum=now;
    }
    // printf("ieee1888_datapool_mgmt_thread_vacuum_mgmt (end)\n");

    //
    // Retry Management Process
    // 
    // printf("ieee1888_datapool_mgmt_thread_retry_mgmt (begin)\n");

    pthread_mutex_lock(&ieee1888_datapool_mx); // -- begin datapool lock

    // Prepare Buffer file
    char buffer_path[200];
    sprintf(buffer_path,"%s.buf",ieee1888_datapool_path);
    FILE* fp_buffer=NULL;

    // Start Reading 
    FILE* fp=fopen(ieee1888_datapool_path,"r");
    if(fp==NULL){
      pthread_mutex_unlock(&ieee1888_datapool_mx); // -- end datapool lock
      // printf("ieee1888_datapool_mgmt_thread_retry_mgmt (no datapool file)\n"); // this is not an error
      // printf("ieee1888_datapool_mgmt_thread_retry_mgmt (end)\n");
      sleep(600);
      continue;
    }
    row_index=0;
    while(fgets(line,1024,fp)!=NULL){
      if(row_index<row_limit){
        if(ieee1888_datapool_parse_CSV_line(line,row_id,row_time,row_value)){
          // pushes datapool (maximum row_limit lines)
          ieee1888_datapool_create_dataRQ_push(row_id,row_time,row_value);
          row_index++;
        }
      }else{
        if(fp_buffer==NULL){
          fp_buffer=fopen(buffer_path,"w");
        }
        if(fp_buffer!=NULL){
          fputs(line,fp_buffer);
        }
      }
    }
    fclose(fp);
    if(fp_buffer!=NULL){
      fclose(fp_buffer);
    }

    // for mutual exclusion test
    // printf("retry wait begin...\n");
    // sleep(60);
    // printf("retry wait end...\n");

    if(ieee1888_datapool_try_dataRQ()!=IEEE1888_DATAPOOL_IEEE1888_TRANSPORT_ERROR){
      // clear the point tree cache
      ieee1888_datapool_create_dataRQ_clearCache();

      // copy back (begin)
      FILE* fp_buffer=fopen(buffer_path,"r");
      if(fp_buffer!=NULL){
        FILE* fp=fopen(ieee1888_datapool_path,"w");
        if(fp!=NULL){
          while(fgets(line,1024,fp_buffer)!=NULL){
            fputs(line,fp);
          }
          fclose(fp);
          fclose(fp_buffer);
          remove(buffer_path);
          pthread_mutex_unlock(&ieee1888_datapool_mx); // -- end datapool lock

          // NORMALLY SUCCESS --> row_limit=1000, sleep=60 sec
          // printf("ieee1888_datapool_mgmt_thread_retry_mgmt: normally success; next row_limit=1000 sleep=60\n");
          row_limit=1000;
          // printf("ieee1888_datapool_mgmt_thread_retry_mgmt (end)\n");
          sleep(60);
        }else{
          fclose(fp_buffer);
          remove(buffer_path);
          pthread_mutex_unlock(&ieee1888_datapool_mx); // -- end datapool lock

          // DISK ERROR sleep=60 sec
          printf("ieee1888_datapool_mgmt_thread_retry_mgmt: Error while copying back from the buffer to the main datapool -- failed to open datapool file\n");
          // printf("ieee1888_datapool_mgmt_thread_retry_mgmt: sleep=60\n");
          // printf("ieee1888_datapool_mgmt_thread_retry_mgmt (end)\n");
          sleep(60);
        }
      }else{
        remove(ieee1888_datapool_path);
        pthread_mutex_unlock(&ieee1888_datapool_mx); // -- end datapool lock

        // EMPTY SUCCESS --> row_limit=10, sleep=600 sec
        // printf("ieee1888_datapool_mgmt_thread_retry_mgmt: emtpy success; next row_limit=10 sleep=600\n");
        row_limit=10;
        // printf("ieee1888_datapool_mgmt_thread_retry_mgmt (end)\n");
        sleep(600);
      }
      // copy back (end)

    }else{
      // clear the point tree cache
      ieee1888_datapool_create_dataRQ_clearCache();

      remove(buffer_path);
      pthread_mutex_unlock(&ieee1888_datapool_mx); // -- end datapool lock

      // TRANSPORT ERROR --> row_limit=10, sleep=600 sec
      // printf("ieee1888_datapool_mgmt_thread_retry_mgmt: transport error; next row_limit=10 sleep=600\n"); // this is not an error
      row_limit=10;
      // printf("ieee1888_datapool_mgmt_thread_retry_mgmt (end)\n");
      sleep(600);
    }
  }
}

void ieee1888_datapool_init(const char* datapool_path, const char* ieee1888_server_url, int timespan_min){

  // printf("ieee1888_datapool_init(begin)\n");

  if(timespan_min<=0){
    ieee1888_datapool_timespan_min=0;
  }else if(timespan_min>7*24*60){
    ieee1888_datapool_timespan_min=7*24*60;
  }else{
    ieee1888_datapool_timespan_min=timespan_min;
  }

  strncpy(ieee1888_datapool_path,datapool_path,IEEE1888_DATAPOOL_PATH_LEN);
  strncpy(ieee1888_datapool_IEEE1888_SERVER_URL,ieee1888_server_url,IEEE1888_DATAPOOL_IEEE1888_SERVER_URL_LEN);

  pthread_create(&__ieee1888_datapool_mgmt_thread,0,ieee1888_datapool_mgmt_thread,0);
  pthread_mutex_init(&ieee1888_datapool_mx,0);

  // printf("ieee1888_datapool_init(end)\n");
}
