/*
 * ieee1888_client.c
 *
 * author: Hideya Ochiai
 * create: 2011-11-30
 * update: 2012-12-08
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>

#include "ieee1888.h"
#include "ieee1888_XMLparser.h"
#include "ieee1888_XMLgenerator.h"

struct s_opt {
  sa_family_t family;
  int debug;
  char* host;
  char* service;
};

#define IEEE1888_SOAP_MSG_BUFFER_SIZE 524228

#define IEEE1888_SOAP_CLIENT_IMPL_OK                 0
#define IEEE1888_SOAP_CLIENT_IMPL_ERROR_SCHEMA       1
#define IEEE1888_SOAP_CLIENT_IMPL_ERROR_DNS_RESOLVE  2
#define IEEE1888_SOAP_CLIENT_IMPL_ERROR_SOCKET_IO    3
#define IEEE1888_SOAP_CLIENT_IMPL_ERROR_HTTP         4
#define IEEE1888_SOAP_CLIENT_IMPL_ERROR_OVERFLOW     5

int ieee1888_soap_client(const char* soapEPR, const char* send_msg, char* recv_msg, int n_recv_msg, const char* soapAction, const char* userAgent){

  int i;
  int max_epr_len=strlen(soapEPR)+1;

  /* ---- Step 1 : parse soapEPR (begin) ---- */
  // schema check
  if(strncmp(soapEPR,"http://",7)!=0){
    fprintf(stderr,"Fatal error. The specified communication schema is not supported.");
    fflush(stderr);
    return IEEE1888_SOAP_CLIENT_IMPL_ERROR_SCHEMA;
  }
  char* buffer=(char*)malloc(max_epr_len);
  char* host=(char*)malloc(max_epr_len);
  char* dir=(char*)malloc(max_epr_len);
  int port=80;
  const char* p=soapEPR+7;
  char* q=buffer;

  // parse hostname
  for(i=0;*p!=':' && *p!='/' && *p!='\0' && i<max_epr_len;i++, q++, p++){
    *q=*p;
  }
  *q='\0';
  strcpy(host,buffer);

  // parse port number
  q=buffer;
  if(*p==':'){
    p++;
    for(i=0;*p!='/' && *p!='\0' && i<max_epr_len;i++, q++, p++){
      *q=*p;
    }
    *q='\0';
    port=atoi(buffer);
  }
  
  // parse dir
  q=buffer;
  if(*p=='/'){
    for(i=0;*p!='\0' && i<max_epr_len;i++, q++, p++){
      *q=*p;
    }
    *q='\0';
    strcpy(dir,buffer);
  }else{
    *dir='\0';
  }

  free(buffer);

  // for debug
  /*
  printf("host: %s\n",host);
  printf("port: %d\n",port);
  printf("dir:  %s\n",dir);
  */
  /* ---- Step 1 : parse soapEPR (end) ---- */
  
  /* ---- Step 2 : create connection to the server (begin) ---- */

  // tolerance for broken pipes 
  signal(SIGPIPE,SIG_IGN);

  int s=0;
  int err;
  struct s_opt sopt;
  struct addrinfo hints, *res, *res0;
  // char* serv;
  char str_port[10];

  sprintf(str_port,"%d",port);
	
  memset(&sopt, 0, sizeof(sopt));
  sopt.family=PF_UNSPEC;
  sopt.host=host;
  sopt.service=str_port;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family=PF_UNSPEC;
  hints.ai_socktype=SOCK_STREAM;

  err=getaddrinfo(sopt.host, sopt.service, &hints, &res0);
  if(err){
    fprintf(stderr,"ERROR: getaddrinfo(%d): %s\n", err, gai_strerror(err)); fflush(stderr);
    // free & return 
    free(host); free(dir);
    return IEEE1888_SOAP_CLIENT_IMPL_ERROR_DNS_RESOLVE;
  }
	
  for(res=res0; res; res=res->ai_next){
    s=socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(s<0){
      fprintf(stderr,"ERROR: socket open()\n"); fflush(stderr);
      continue;
    }
    if(connect(s, res->ai_addr, res->ai_addrlen)<0){
      fprintf(stderr,"ERROR: socket connect()\n"); fflush(stderr);
      close(s);
      s=-1;
      continue;
    }
    if(sopt.debug){
      getnameinfo(res->ai_addr,res->ai_addrlen,
                  host, sizeof(host),
                  str_port, sizeof(str_port),
                  NI_NUMERICHOST|NI_NUMERICSERV);
      fprintf(stderr,"DEBUG: connect: [%s]:%s\n",host,str_port);
    }
    break;
  }
  
  freeaddrinfo(res0);

  if(s<0){
    fprintf(stderr,"ERROR: cannot connect %s\n",sopt.host);
    fflush(stderr);
    // free & return
    free(host); free(dir);
    return IEEE1888_SOAP_CLIENT_IMPL_ERROR_SOCKET_IO;
  }

  // Body construction
  int n_send_msg=strlen(send_msg);

  // Header construction
  int h=0;
  int n_userAgent=0;
  int n_soapAction=0;
  if(userAgent!=NULL){ n_userAgent=strlen(userAgent); }
  if(soapAction!=NULL){ n_soapAction=strlen(soapAction); }
  char* requestHeader=malloc(400+n_userAgent+n_soapAction);

  h+=sprintf(&requestHeader[h],"POST %s HTTP/1.1\r\n",dir); 
  h+=sprintf(&requestHeader[h],"Content-Type: text/xml;charset=UTF-8\r\n");
  if(userAgent!=NULL){
    h+=sprintf(&requestHeader[h],"User-Agent: %s\r\n",userAgent);
  }
  h+=sprintf(&requestHeader[h],"Host: %s\r\n",host);
  if(soapAction!=NULL){
    h+=sprintf(&requestHeader[h],"SOAPAction: \"%s\"\r\n",soapAction);
  }
  h+=sprintf(&requestHeader[h],"Content-Length: %d\r\n",n_send_msg+4);
  h+=sprintf(&requestHeader[h],"\r\n");

  // send header, msg_body, and \r\n \r\n
  send(s,requestHeader,h,0);
  send(s,send_msg,n_send_msg,0);
  send(s,"\r\n\r\n",4,0);

  // for Debug
  // printf(requestHeader);
  // printf(send_msg);
  // printf("\r\n\r\n");
  
  // int ret_code=IEEE1888_SOAP_CLIENT_IMPL_OK;
  int parse_header=1;
  int needle=0;
  int offset=0;
  char http_recv_buffer[65536];
  char line[65536];

  int finish=0;

  // content of the header
  int http_success=0;
  int http_overflow=0;
  int http_transfer_encoding=0;
  int http_content_length=0;
  int http_next_transfer_length=0;
  int http_next_transfer_body=0;
  char* p_recv_msg=recv_msg;
  // int l_recv_msg=n_recv_msg;
  int r_recv_msg=0;   // total received;

  while(1){
    int c=recv(s,http_recv_buffer+offset,65536-offset,0);
    if(c>0){
      http_recv_buffer[c+offset]=0;
      needle=0;
      
      // for Debug
      // printf(&http_recv_buffer[offset]);

      while(1){

        // get a line (if failed, recv(...) next.)
        char* p;
        char* q;
        p=http_recv_buffer+needle;
        q=strstr(p,"\r\n");
	int n=q-p;
        if(q!=NULL){
          int i;
          for(i=0;i<n;i++){
            strncpy(line,p,n);
          }
	  line[n]='\0';
          needle+=n+2;
        }else{
	  q=http_recv_buffer;
	  offset=0;
	  while(*p!=0){
	    *(q++)=*(p++);
	    offset++;
	  }
	  break;
        }

	if(parse_header){  // parse header lines
          if(n>0){
            if(strstr(line,"HTTP")==line){
	      if(strstr(line,"200 OK")!=NULL){
                http_success=1;
	      }else{
                http_success=0;
	      }
	    }else if(strstr(line,"Transfer-Encoding: ")==line){
	      if(strstr(line,"chunked")!=NULL){
                http_transfer_encoding=1;
	      }
	    }else if(strstr(line,"Content-Length: ")==line){
	      char* p=strstr(line,":");
	      if(p!=NULL){
	        http_content_length=atoi(p+2);
	      }
	    }
	  }else{
	    parse_header=0;
	  }

	}else{  // parse body lines
	  
	  if(http_transfer_encoding){  // for Transfer-Encoding: chunked
	    if(!http_next_transfer_body){
              // calc length
	      int len=0;
	      // char buf[2];
	      int i=0;
	      for(i=0;i<20 && line[i]!='\0';i++){
	        len*=16;
                if('0'<= line[i] && line[i]<='9'){
		  len+=(line[i]-'0');
		}else if(line[i]=='a' || line[i]=='A'){
		  len+=10;
		}else if(line[i]=='b' || line[i]=='B'){
		  len+=11;
		}else if(line[i]=='c' || line[i]=='C'){
		  len+=12;
		}else if(line[i]=='d' || line[i]=='D'){
		  len+=13;
		}else if(line[i]=='e' || line[i]=='E'){
		  len+=14;
		}else if(line[i]=='f' || line[i]=='F'){
		  len+=15;
		}
	      }
              http_next_transfer_length=len;
	      http_next_transfer_body=1;
	    }else{
	      if(http_next_transfer_length!=0){
                // copy the body
		if(r_recv_msg+http_next_transfer_length<n_recv_msg){
                  strncpy(p_recv_msg,line,http_next_transfer_length);
		  p_recv_msg+=http_next_transfer_length;
		  r_recv_msg+=http_next_transfer_length;
                  http_next_transfer_body=0;
		}else{
                  finish=1;
		  *p_recv_msg='\0';
		  http_overflow=1;
		}
              }else{
	        // finish
                finish=1;
		*p_recv_msg='\0';
	      }
	    }

	  }else{  // for Content-Length: ...
	    
	    int len=strlen(line);
	    if(http_content_length>n_recv_msg
	       || (len+r_recv_msg)>n_recv_msg){

	      http_overflow=1;
	      finish=1;
	      *p_recv_msg='\0';

	    }else{

	      strcpy(p_recv_msg,line);
	      p_recv_msg+=len;
	      r_recv_msg+=len;
	      if(r_recv_msg>=http_content_length){
                finish=1;
	        *p_recv_msg='\0';
              }

	    }
	  }
	}
      }
      if(finish){
        break;
      }

    }else if(c==0){
      break;
    }else if(c<0){
      break;
    }
  }
  close(s);

  free(requestHeader);
  free(host);
  free(dir);
  
  if(!http_success){
    return IEEE1888_SOAP_CLIENT_IMPL_ERROR_HTTP;
  }
  if(http_overflow){
    return IEEE1888_SOAP_CLIENT_IMPL_ERROR_OVERFLOW;
  }
  return IEEE1888_SOAP_CLIENT_IMPL_OK;
}

ieee1888_transport* ieee1888_client_query(ieee1888_transport* request, const char* soapEPR, const char** options, int* err){

  int res;
  char send_msg[IEEE1888_SOAP_MSG_BUFFER_SIZE];
  char recv_msg[IEEE1888_SOAP_MSG_BUFFER_SIZE];

  res=ieee1888_soap_gen(request, IEEE1888_QUERY_RQ, send_msg, IEEE1888_SOAP_MSG_BUFFER_SIZE);
  if(res<0){
    switch(res){
    case IEEE1888_SOAP_GEN_ERROR_UNKNOWN_MSG:
      if(err!=NULL){ 
        *err=IEEE1888_CLIENT_ERROR_INTERNAL; 
      }
      return NULL;

    case IEEE1888_SOAP_GEN_ERROR_OVERFLOW:
      if(err!=NULL){
        *err=IEEE1888_CLIENT_ERROR_BUFFER_OVERFLOW;
      }
      return NULL;

    default:
      if(err!=NULL){ 
        *err=IEEE1888_CLIENT_ERROR_INTERNAL; 
      }
      return NULL;
    }
  }


  res=ieee1888_soap_client(soapEPR,
                           send_msg,
                           recv_msg,
                           IEEE1888_SOAP_MSG_BUFFER_SIZE,
                           "http://soap.fiap.org/query",
	                   "IEEE1888_C_STACK_20121208");

  switch(res){
  case IEEE1888_SOAP_CLIENT_IMPL_OK:
    if(err!=NULL){
      *err=IEEE1888_CLIENT_OK;
    }
    break;

  case IEEE1888_SOAP_CLIENT_IMPL_ERROR_SCHEMA:
    if(err!=NULL){
      *err=IEEE1888_CLIENT_ERROR_COMM_SCHEMA;
    }
    return NULL;

  case IEEE1888_SOAP_CLIENT_IMPL_ERROR_DNS_RESOLVE:
    if(err!=NULL){
      *err=IEEE1888_CLIENT_ERROR_DNS_RESOLVE;
    }
    return NULL;

  case IEEE1888_SOAP_CLIENT_IMPL_ERROR_SOCKET_IO:
    if(err!=NULL){
      *err=IEEE1888_CLIENT_ERROR_SOCKET;
    }
    return NULL;

  case IEEE1888_SOAP_CLIENT_IMPL_ERROR_HTTP:
    if(err!=NULL){
      *err=IEEE1888_CLIENT_ERROR_HTTP;
    }
    return NULL;

  case IEEE1888_SOAP_CLIENT_IMPL_ERROR_OVERFLOW:
    if(err!=NULL){
      *err=IEEE1888_CLIENT_ERROR_BUFFER_OVERFLOW;
    }
    return NULL;

  default:
    if(err!=NULL){
      *err=IEEE1888_CLIENT_ERROR_INTERNAL;
    }
    return NULL;
  }

  int message;
  ieee1888_transport* response=ieee1888_mk_transport();
  int len=strlen(recv_msg);
  ieee1888_soap_parse(recv_msg, len, response, &message);

  if(message==IEEE1888_QUERY_RS){
    return response;
  }

  if(err!=NULL){
    *err=IEEE1888_CLIENT_ERROR_INTERNAL;
  }
  return NULL;
}

ieee1888_transport* ieee1888_client_data(ieee1888_transport* request, const char* soapEPR, const char** options, int* err){
  
  int res;
  char send_msg[IEEE1888_SOAP_MSG_BUFFER_SIZE];
  char recv_msg[IEEE1888_SOAP_MSG_BUFFER_SIZE];

  res=ieee1888_soap_gen(request, IEEE1888_DATA_RQ, send_msg, IEEE1888_SOAP_MSG_BUFFER_SIZE);
  if(res<0){
    switch(res){
    case IEEE1888_SOAP_GEN_ERROR_UNKNOWN_MSG:
      if(err!=NULL){
        *err=IEEE1888_CLIENT_ERROR_INTERNAL;
      }
      return NULL;

    case IEEE1888_SOAP_GEN_ERROR_OVERFLOW:
      if(err!=NULL){
        *err=IEEE1888_CLIENT_ERROR_BUFFER_OVERFLOW;
      }
      return NULL;

    default:
      if(err!=NULL){
        *err=IEEE1888_CLIENT_ERROR_INTERNAL;
      }
      return NULL;
    }
  }
  
  res=ieee1888_soap_client(soapEPR,
                           send_msg,
	                   recv_msg,
		           IEEE1888_SOAP_MSG_BUFFER_SIZE,
		           "http://soap.fiap.org/data",
		           "IEEE1888_C_STACK_20121208");

  switch(res){
  case IEEE1888_SOAP_CLIENT_IMPL_OK:
    if(err!=NULL){
      *err=IEEE1888_CLIENT_OK;
    }
    break;

  case IEEE1888_SOAP_CLIENT_IMPL_ERROR_SCHEMA:
    if(err!=NULL){
      *err=IEEE1888_CLIENT_ERROR_COMM_SCHEMA;
    }
    return NULL;

  case IEEE1888_SOAP_CLIENT_IMPL_ERROR_DNS_RESOLVE:
    if(err!=NULL){
      *err=IEEE1888_CLIENT_ERROR_DNS_RESOLVE;
    }
    return NULL;

  case IEEE1888_SOAP_CLIENT_IMPL_ERROR_SOCKET_IO:
    if(err!=NULL){
      *err=IEEE1888_CLIENT_ERROR_SOCKET;
    }
    return NULL;

  case IEEE1888_SOAP_CLIENT_IMPL_ERROR_HTTP:
    if(err!=NULL){
      *err=IEEE1888_CLIENT_ERROR_HTTP;
    }
    return NULL;

  case IEEE1888_SOAP_CLIENT_IMPL_ERROR_OVERFLOW:
    if(err!=NULL){
      *err=IEEE1888_CLIENT_ERROR_BUFFER_OVERFLOW;
    }
    return NULL;

  default:
    if(err!=NULL){
      *err=IEEE1888_CLIENT_ERROR_INTERNAL;
    }
    return NULL;
  }

  int message;
  ieee1888_transport* response=ieee1888_mk_transport();

  int len=strlen(recv_msg);
  ieee1888_soap_parse(recv_msg, len, response, &message);

  if(message==IEEE1888_DATA_RS){
    return response;
  }

  if(err!=NULL){
    *err=IEEE1888_CLIENT_ERROR_INTERNAL;
  }
  return NULL;
}

/*
int main(int argc, char* argv){

   char line[1000];

   int n_buffer=0;
   char buffer[100000];

   FILE* fp=fopen("queryRQ.xml","r");

   while(fgets(line,1000,fp)!=NULL){
     int len=strlen(line);
     strcpy(&buffer[n_buffer],line);
     n_buffer+=len;
   }
   fclose(fp);

   const char* p=buffer;
   const char* q; 
   ieee1888_transport* transport=ieee1888_mk_transport();
   ieee1888_parseXML_transport(p,transport);
   ieee1888_dump_objects((ieee1888_object*)transport);
  
   ieee1888_transport* res=ieee1888_client_query(transport,"http://fiap-develop.gutp.ic.i.u-tokyo.ac.jp/axis2/services/FIAPStorage");

   
   ieee1888_dump_objects((ieee1888_object*)res);

   return 1;
}
*/
