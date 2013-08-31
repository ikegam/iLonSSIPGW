/*
 * ieee1888_server.c -- IEEE1888 server stub implementation (only Component-to-Component communication)
 *                      (Multiple Thread Supported)
 * 
 * create: 2011-11-01
 * update: 2012-12-08
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <netdb.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "ieee1888.h"
#include "ieee1888_XMLgenerator.h"
#include "ieee1888_XMLparser.h"

// TODO: configulation parameters (-> ieee1888_config.h)
#define IEEE1888_EPR_PATH "/IEEE1888GW"
#define IEEE1888_WSDL_PATH "/IEEE1888GW?wsdl"
#define LOCAL_ROOT_DIR "/var/www"

#define SEND_MSG_BUFSIZE 262144
#define RECV_MSG_BUFSIZE 262144
#define RECV_MSG_LINESIZE 262000

#define HTTP_METHOD_HEAD    1
#define HTTP_METHOD_GET     2
#define HTTP_METHOD_POST    3
#define HTTP_METHOD_OTHERS 99
struct s_opt {
  sa_family_t family;
  int debug;
  char* host;
  char* service;
};

// callback method registers (for query and data)
static ieee1888_transport* (*__ieee1888_server_query)(ieee1888_transport* request, char** params)=NULL;
static ieee1888_transport* (*__ieee1888_server_data)(ieee1888_transport* request, char** params)=NULL;

// registation of the callback methods
void ieee1888_set_service_handlers(ieee1888_transport* (*query)(ieee1888_transport* request, char** params), ieee1888_transport* (*data)(ieee1888_transport* request, char** params)){
  __ieee1888_server_query=query;
  __ieee1888_server_data=data;
}

// Be careful (if you are going to call this method)
//   -- storage size of buffer must be more than 90000 bytes.
int ieee1888_server_print_wsdl(char* buffer, int socket, const char* path){
 
  int len;
  char host[100];
  char serv[10];
  struct sockaddr_storage sa;
  len=sizeof(sa);
  if(getsockname(socket, (struct sockaddr*)&sa, &len)==0){
     char buf[100];
     getnameinfo((struct sockaddr*)&sa,len,buf,100,serv,10,NI_NOFQDN|NI_NUMERICHOST|NI_NUMERICSERV);
     if(strstr(buf,".")!=NULL){
       if(strstr(buf,"ffff:")!=NULL){
         strcpy(host,strstr(buf,"ffff:")+5);
       }else{
         strcpy(host,buf);
       }
     }else{
       sprintf(host,"[%s]",buf);
     }
  }

  int l=0;
  // char ipaddr[30]="";
  l+=sprintf(&buffer[l],"<?xml version=\"1.0\" encoding=\"utf-8\"?>");
  l+=sprintf(&buffer[l],"<wsdl:definitions xmlns:s0=\"http://gutp.jp/fiap/2009/11/\"");
  l+=sprintf(&buffer[l]," xmlns:soap=\"http://schemas.xmlsoap.org/wsdl/soap/\"");
  l+=sprintf(&buffer[l]," xmlns:s=\"http://www.w3.org/2001/XMLSchema\"");
  l+=sprintf(&buffer[l]," xmlns:http=\"http://schemas.xmlsoap.org/wsdl/http/\"");
  l+=sprintf(&buffer[l]," xmlns:tns=\"http://soap.fiap.org/\" ");
  l+=sprintf(&buffer[l]," targetNamespace=\"http://soap.fiap.org/\"");
  l+=sprintf(&buffer[l]," xmlns:wsdl=\"http://schemas.xmlsoap.org/wsdl/\">");

  l+=sprintf(&buffer[l],"<wsdl:types>");
  l+=sprintf(&buffer[l],"<s:schema elementFormDefault=\"qualified\" targetNamespace=\"http://gutp.jp/fiap/2009/11/\">");
  l+=sprintf(&buffer[l],"<s:simpleType name=\"uuid\">");
  l+=sprintf(&buffer[l],"<s:restriction base=\"s:string\">");
  l+=sprintf(&buffer[l],"<s:pattern value=\"[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}\"/>");
  l+=sprintf(&buffer[l],"</s:restriction>");
  l+=sprintf(&buffer[l],"</s:simpleType>");

  l+=sprintf(&buffer[l],"<s:simpleType name=\"queryType\">");
  l+=sprintf(&buffer[l],"<s:restriction base=\"s:string\">");
  l+=sprintf(&buffer[l],"<s:enumeration value=\"storage\"/>");
  l+=sprintf(&buffer[l],"<s:enumeration value=\"stream\"/>");
  l+=sprintf(&buffer[l],"</s:restriction>");
  l+=sprintf(&buffer[l],"</s:simpleType>");

  l+=sprintf(&buffer[l],"<s:simpleType name=\"attrNameType\">");
  l+=sprintf(&buffer[l],"<s:restriction base=\"s:string\">");
  l+=sprintf(&buffer[l],"<s:enumeration value=\"time\"/>");
  l+=sprintf(&buffer[l],"<s:enumeration value=\"value\"/>");
  l+=sprintf(&buffer[l],"</s:restriction>");
  l+=sprintf(&buffer[l],"</s:simpleType>");

  l+=sprintf(&buffer[l],"<s:simpleType name=\"selectType\">");
  l+=sprintf(&buffer[l],"<s:restriction base=\"s:string\">");
  l+=sprintf(&buffer[l],"<s:enumeration value=\"minimum\"/>");
  l+=sprintf(&buffer[l],"<s:enumeration value=\"maximum\"/>");
  l+=sprintf(&buffer[l],"</s:restriction>");
  l+=sprintf(&buffer[l],"</s:simpleType>");
  l+=sprintf(&buffer[l],"<s:simpleType name=\"trapType\">");
  l+=sprintf(&buffer[l],"<s:restriction base=\"s:string\">");
  l+=sprintf(&buffer[l],"<s:enumeration value=\"changed\"/>");
  l+=sprintf(&buffer[l],"</s:restriction>");
  l+=sprintf(&buffer[l],"</s:simpleType>");

  l+=sprintf(&buffer[l],"<s:complexType name=\"key\">");
  l+=sprintf(&buffer[l],"<s:sequence>");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"0\" maxOccurs=\"unbounded\" name=\"key\" type=\"s0:key\" />");
  l+=sprintf(&buffer[l],"</s:sequence>");
  l+=sprintf(&buffer[l],"<s:attribute name=\"id\" type=\"s:anyURI\" use=\"required\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"attrName\" type=\"s0:attrNameType\" use=\"required\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"eq\" type=\"s:string\" use=\"optional\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"neq\" type=\"s:string\" use=\"optional\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"lt\" type=\"s:string\" use=\"optional\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"gt\" type=\"s:string\" use=\"optional\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"lteq\" type=\"s:string\" use=\"optional\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"gteq\" type=\"s:string\" use=\"optional\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"select\" type=\"s0:selectType\" use=\"optional\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"trap\" type=\"s0:trapType\" use=\"optional\" />");
  l+=sprintf(&buffer[l],"</s:complexType>");

  l+=sprintf(&buffer[l],"<s:complexType name=\"query\">");
  l+=sprintf(&buffer[l],"<s:sequence>");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"0\" maxOccurs=\"unbounded\" name=\"key\" type=\"s0:key\" />");
  l+=sprintf(&buffer[l],"</s:sequence>");
  l+=sprintf(&buffer[l],"<s:attribute name=\"id\" type=\"s0:uuid\" use=\"required\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"type\" type=\"s0:queryType\" use=\"required\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"cursor\" type=\"s0:uuid\" use=\"optional\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"ttl\" type=\"s:nonNegativeInteger\" use=\"optional\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"acceptableSize\" type=\"s:positiveInteger\" use=\"optional\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"callbackData\" type=\"s:anyURI\" use=\"optional\" />");
  l+=sprintf(&buffer[l],"<s:attribute name=\"callbackControl\" type=\"s:anyURI\" use=\"optional\" />");
  l+=sprintf(&buffer[l],"</s:complexType>");
           
  l+=sprintf(&buffer[l],"<s:complexType name=\"error\">");
  l+=sprintf(&buffer[l],"<s:simpleContent>");
  l+=sprintf(&buffer[l],"<s:extension base=\"s:string\">");
  l+=sprintf(&buffer[l],"<s:attribute name=\"type\" type=\"s:string\" use=\"required\" />");
  l+=sprintf(&buffer[l],"</s:extension>");
  l+=sprintf(&buffer[l],"</s:simpleContent>");
  l+=sprintf(&buffer[l],"</s:complexType>");

  l+=sprintf(&buffer[l],"<s:complexType name=\"OK\">");
  l+=sprintf(&buffer[l],"</s:complexType>");

  l+=sprintf(&buffer[l],"<s:complexType name=\"header\">");
  l+=sprintf(&buffer[l],"<s:sequence>");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"0\" maxOccurs=\"1\" name=\"OK\" type=\"s0:OK\" />");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"0\" maxOccurs=\"1\" name=\"error\" type=\"s0:error\" />");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"0\" maxOccurs=\"1\" name=\"query\" type=\"s0:query\" />");
  l+=sprintf(&buffer[l],"</s:sequence>");
  l+=sprintf(&buffer[l],"</s:complexType>");

  l+=sprintf(&buffer[l],"<s:complexType name=\"value\">");
  l+=sprintf(&buffer[l],"<s:simpleContent>");
  l+=sprintf(&buffer[l],"<s:extension base=\"s:string\">");
  l+=sprintf(&buffer[l],"<s:attribute name=\"time\" type=\"s:dateTime\" use=\"optional\" />");
  l+=sprintf(&buffer[l],"</s:extension>");
  l+=sprintf(&buffer[l],"</s:simpleContent>");
  l+=sprintf(&buffer[l],"</s:complexType>");
      
  l+=sprintf(&buffer[l],"<s:complexType name=\"point\">");
  l+=sprintf(&buffer[l],"<s:sequence>");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"0\" maxOccurs=\"unbounded\" name=\"value\" type=\"s0:value\" />");
  l+=sprintf(&buffer[l],"</s:sequence>");
  l+=sprintf(&buffer[l],"<s:attribute name=\"id\" type=\"s:anyURI\" use=\"required\" />");
  l+=sprintf(&buffer[l],"</s:complexType>");

  l+=sprintf(&buffer[l],"<s:complexType name=\"pointSet\">");
  l+=sprintf(&buffer[l],"<s:sequence>");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"0\" maxOccurs=\"unbounded\" name=\"pointSet\" type=\"s0:pointSet\" />");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"0\" maxOccurs=\"unbounded\" name=\"point\" type=\"s0:point\" />");
  l+=sprintf(&buffer[l],"</s:sequence>");
  l+=sprintf(&buffer[l],"<s:attribute name=\"id\" type=\"s:anyURI\" use=\"required\" />");
  l+=sprintf(&buffer[l],"</s:complexType>");

  l+=sprintf(&buffer[l],"<s:complexType name=\"body\">");
  l+=sprintf(&buffer[l],"<s:sequence>");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"0\" maxOccurs=\"unbounded\" name=\"pointSet\" type=\"s0:pointSet\"/>");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"0\" maxOccurs=\"unbounded\" name=\"point\" type=\"s0:point\"/>");
  l+=sprintf(&buffer[l],"</s:sequence>");
  l+=sprintf(&buffer[l],"</s:complexType>");

  l+=sprintf(&buffer[l],"<s:complexType name=\"transport\">");
  l+=sprintf(&buffer[l],"<s:sequence>");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"0\" maxOccurs=\"1\" name=\"header\" type=\"s0:header\" />");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"0\" maxOccurs=\"1\" name=\"body\" type=\"s0:body\" />");
  l+=sprintf(&buffer[l],"</s:sequence>");
  l+=sprintf(&buffer[l],"</s:complexType>");

  l+=sprintf(&buffer[l],"<s:element name=\"transport\" type=\"s0:transport\" />");
  l+=sprintf(&buffer[l],"</s:schema>");

  l+=sprintf(&buffer[l],"<s:schema elementFormDefault=\"qualified\" targetNamespace=\"http://soap.fiap.org/\">");
  l+=sprintf(&buffer[l],"<s:import namespace=\"http://gutp.jp/fiap/2009/11/\" />");

  l+=sprintf(&buffer[l],"<s:element name=\"queryRQ\">");
  l+=sprintf(&buffer[l],"<s:complexType>");
  l+=sprintf(&buffer[l],"<s:sequence>");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"1\" maxOccurs=\"1\" ref=\"s0:transport\" />");
  l+=sprintf(&buffer[l],"</s:sequence>");
  l+=sprintf(&buffer[l],"</s:complexType>");
  l+=sprintf(&buffer[l],"</s:element>");
      
  l+=sprintf(&buffer[l],"<s:element name=\"queryRS\">");
  l+=sprintf(&buffer[l],"<s:complexType>");
  l+=sprintf(&buffer[l],"<s:sequence>");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"1\" maxOccurs=\"1\" ref=\"s0:transport\" />");
  l+=sprintf(&buffer[l],"</s:sequence>");
  l+=sprintf(&buffer[l],"</s:complexType>");
  l+=sprintf(&buffer[l],"</s:element>");
      
  l+=sprintf(&buffer[l],"<s:element name=\"dataRQ\">");
  l+=sprintf(&buffer[l],"<s:complexType>");
  l+=sprintf(&buffer[l],"<s:sequence>");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"1\" maxOccurs=\"1\" ref=\"s0:transport\" />");
  l+=sprintf(&buffer[l],"</s:sequence>");
  l+=sprintf(&buffer[l],"</s:complexType>");
  l+=sprintf(&buffer[l],"</s:element>");
      
  l+=sprintf(&buffer[l],"<s:element name=\"dataRS\">");
  l+=sprintf(&buffer[l],"<s:complexType>");
  l+=sprintf(&buffer[l],"<s:sequence>");
  l+=sprintf(&buffer[l],"<s:element minOccurs=\"1\" maxOccurs=\"1\" ref=\"s0:transport\" />");
  l+=sprintf(&buffer[l],"</s:sequence>");
  l+=sprintf(&buffer[l],"</s:complexType>");
  l+=sprintf(&buffer[l],"</s:element>");

  l+=sprintf(&buffer[l],"</s:schema>");

  l+=sprintf(&buffer[l],"</wsdl:types>");
  l+=sprintf(&buffer[l],"<wsdl:message name=\"querySoapIn\">");
  l+=sprintf(&buffer[l],"<wsdl:part name=\"parameters\" element=\"tns:queryRQ\" />");
  l+=sprintf(&buffer[l],"</wsdl:message>");
  l+=sprintf(&buffer[l],"<wsdl:message name=\"querySoapOut\">");
  l+=sprintf(&buffer[l],"<wsdl:part name=\"parameters\" element=\"tns:queryRS\" />");
  l+=sprintf(&buffer[l],"</wsdl:message>");
  l+=sprintf(&buffer[l],"<wsdl:message name=\"dataSoapIn\">");
  l+=sprintf(&buffer[l],"<wsdl:part name=\"parameters\" element=\"tns:dataRQ\" />");
  l+=sprintf(&buffer[l],"</wsdl:message>");
  l+=sprintf(&buffer[l],"<wsdl:message name=\"dataSoapOut\">");
  l+=sprintf(&buffer[l],"<wsdl:part name=\"parameters\" element=\"tns:dataRS\" />");
  l+=sprintf(&buffer[l],"</wsdl:message>");

  l+=sprintf(&buffer[l],"<wsdl:portType name=\"FIAPServiceSoap\">");
  l+=sprintf(&buffer[l],"<wsdl:operation name=\"query\">");
  l+=sprintf(&buffer[l],"<wsdl:input message=\"tns:querySoapIn\" />");
  l+=sprintf(&buffer[l],"<wsdl:output message=\"tns:querySoapOut\" />");
  l+=sprintf(&buffer[l],"</wsdl:operation>");

  l+=sprintf(&buffer[l],"<wsdl:operation name=\"data\">");
  l+=sprintf(&buffer[l],"<wsdl:input message=\"tns:dataSoapIn\" />");
  l+=sprintf(&buffer[l],"<wsdl:output message=\"tns:dataSoapOut\" />");
  l+=sprintf(&buffer[l],"</wsdl:operation>");
  l+=sprintf(&buffer[l],"</wsdl:portType>");
  l+=sprintf(&buffer[l],"<wsdl:binding name=\"FIAPServiceSoap\" type=\"tns:FIAPServiceSoap\">");
  l+=sprintf(&buffer[l],"<soap:binding transport=\"http://schemas.xmlsoap.org/soap/http\" />");
  l+=sprintf(&buffer[l],"<wsdl:operation name=\"query\">");
  l+=sprintf(&buffer[l],"<soap:operation soapAction=\"http://soap.fiap.org/query\" style=\"document\" />");
  l+=sprintf(&buffer[l],"<wsdl:input>");
  l+=sprintf(&buffer[l],"<soap:body use=\"literal\" />");
  l+=sprintf(&buffer[l],"</wsdl:input>");
  l+=sprintf(&buffer[l],"<wsdl:output>");
  l+=sprintf(&buffer[l],"<soap:body use=\"literal\" />");
  l+=sprintf(&buffer[l],"</wsdl:output>");
  l+=sprintf(&buffer[l],"</wsdl:operation>");
  l+=sprintf(&buffer[l],"<wsdl:operation name=\"data\">");
  l+=sprintf(&buffer[l],"<soap:operation soapAction=\"http://soap.fiap.org/data\" style=\"document\" />");
  l+=sprintf(&buffer[l],"<wsdl:input>");
  l+=sprintf(&buffer[l],"<soap:body use=\"literal\" />");
  l+=sprintf(&buffer[l],"</wsdl:input>");
  l+=sprintf(&buffer[l],"<wsdl:output>");
  l+=sprintf(&buffer[l],"<soap:body use=\"literal\" />");
  l+=sprintf(&buffer[l],"</wsdl:output>");
  l+=sprintf(&buffer[l],"</wsdl:operation>");
  l+=sprintf(&buffer[l],"</wsdl:binding>");
  l+=sprintf(&buffer[l],"<wsdl:service name=\"IEEE1888\">");
  l+=sprintf(&buffer[l],"<wsdl:port name=\"FIAPServiceSoap\" binding=\"tns:FIAPServiceSoap\">");
  l+=sprintf(&buffer[l],"<soap:address location=\"http://%s:%s%s\" />",host,serv,path);
  l+=sprintf(&buffer[l],"</wsdl:port>");
  l+=sprintf(&buffer[l],"</wsdl:service>");
  l+=sprintf(&buffer[l],"</wsdl:definitions>");
  
  return l;
}


// HTTP processing thread
#define IEEE1888_SERVER_HTTP_SESSION_COUNT 100
#define IEEE1888_SERVER_HTTP_SESSION_TIMEOUT 100
struct __ieee1888_server_http_session {
  int ttl;
  pthread_t thread;
  int socket;
};
struct __ieee1888_server_http_session __ieee1888_session[IEEE1888_SERVER_HTTP_SESSION_COUNT];

pthread_t __ieee1888_server_http_session_mgmt;
pthread_mutex_t __ieee1888_server_http_session_mx;
void* ieee1888_server_http_session_mgmt(){
  int i;
  struct __ieee1888_server_http_session* p;
  while(1){
    pthread_mutex_lock(&__ieee1888_server_http_session_mx);
    for(i=0,p=__ieee1888_session;i<IEEE1888_SERVER_HTTP_SESSION_COUNT;i++,p++){
      if(p->ttl>0){
        p->ttl-=10;
	if(p->ttl<0){
	  p->ttl=0;
	}
      }
    }
    pthread_mutex_unlock(&__ieee1888_server_http_session_mx);
    sleep(10);
  }
}

void* ieee1888_server_http_proc(void* args){

   struct __ieee1888_server_http_session* session=
      (struct __ieee1888_server_http_session*)args;

  int cs=session->socket;

  // transactions
  char* http_recv_buffer=(char*)malloc(RECV_MSG_BUFSIZE);
  int parse_header=1;
  int needle=0;
  int offset=0;
  char* line=(char*)malloc(RECV_MSG_LINESIZE);
  char* recv_msg=(char*)malloc(RECV_MSG_BUFSIZE);
  int n_recv_msg=RECV_MSG_BUFSIZE;

  // if(http_recv_buffer==NULL || line==NULL || recv_msg==NULL){
  //
  // }
	
  // content of the header
  // int http_success=0;
  char http_request_path[256];
  int http_method=0;
  int http_transfer_encoding=0;
  int http_content_length=0;
  int http_next_transfer_length=0;
  int http_next_transfer_body=0;
  int http_next_transfer_received=0;
  char* p_recv_msg=recv_msg;
  int l_recv_msg=n_recv_msg;
  int r_recv_msg=0;   // total received;
  int err_buffer_overrun=0;
  char http_response_code[40];
  char http_content_type[40];
  int is_newline=0;

  int finish=0;
  while(1){
    int c=read(cs,http_recv_buffer+offset,RECV_MSG_BUFSIZE-offset);
    if(c>0){
      http_recv_buffer[c+offset]=0;
      needle=0;

      while(1){
        // get a line (if failed, recv(...) next.)
        char* p;
        char* q;
        p=http_recv_buffer+needle;
        q=strstr(p,"\r\n");
        int n=q-p;
	if(q!=NULL && n>=RECV_MSG_LINESIZE){
	  err_buffer_overrun=1;
	  n=RECV_MSG_LINESIZE-1;
	}
        is_newline=0;
        
	if(q!=NULL){
          strncpy(line,p,n);
          line[n]='\0';
          needle+=n+2;
	  is_newline=1;
	}else if(!parse_header && strlen(p)>0){
          strcpy(line,p);
          needle+=strlen(line);
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
            if(strstr(line,"GET")==line){
              http_method=HTTP_METHOD_GET;
              
	      int j;
	      for(j=4;line[j]!=' ' && j<256;j++){
                http_request_path[j-4]=line[j];
              }
              http_request_path[j-4]='\0';
	    
            }else if(strstr(line,"POST")==line){
              http_method=HTTP_METHOD_POST;

              int j;
              for(j=5;line[j]!=' ' && j<256;j++){
                http_request_path[j-5]=line[j];
              }
	      http_request_path[j-5]='\0';
 
            }else if(strstr(line,"Transfer-Encoding: ")==line){
	      if(strstr(line,"chunked")!=NULL){
                http_transfer_encoding=1;
              }
            }else if(strstr(line,"Content-Length: ")==line){
              char* p=strstr(line,":");
              if(p!=NULL){
                http_content_length=atoi(p+2);
                fflush(stdout);
              }
	    }else if(strstr(line,"Expect: 100-continue")==line){
              char buffer[]="HTTP/1.1 100 Continue\r\n\r\n";
              int len=strlen(buffer);
	      write(cs,buffer,len);
	    }
	  }else{
	    parse_header=0;

	    // printf("parse header ended \n"); fflush(stdout);
	    if(http_method==HTTP_METHOD_GET){
              finish=1;
            }
	  }

	}else{  // parse body lines
	  
          if(http_transfer_encoding){  // for Transfer-Encoding: chunked

            // for debug
	    // char buf[10];
	    // strncpy(buf,line,10);
	    // buf[9]='\0';
	    // printf("M=%d: %s\n",http_next_transfer_body,buf);
	    // fflush(stdout);

            if(!http_next_transfer_body){
              
	      // calc length
              int len=0;
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

	      if(i>0){
                http_next_transfer_length=len;
                http_next_transfer_body=1;
  	        http_next_transfer_received=0;
              }

	    }else{

	      if(http_next_transfer_length!=0){
                // check the remaining length (raises err_buffer_overrun and drop the received data if overrun)
                int copy_length=strlen(line);
	        if(l_recv_msg<http_next_transfer_length+2){ // +2 for \r\n (if is_newline==1)
                  err_buffer_overrun=1;
                  copy_length=l_recv_msg;
		}

                // copy the body
                strncpy(p_recv_msg,line,copy_length);
		p_recv_msg+=copy_length;
		r_recv_msg+=copy_length;
                l_recv_msg-=copy_length;
		http_next_transfer_received+=copy_length;
		if(http_next_transfer_received<http_next_transfer_length
		  && is_newline){
                  strncpy(p_recv_msg,"\r\n",2);
		  p_recv_msg+=2;
		  r_recv_msg+=2;
                  l_recv_msg-=2;
		  http_next_transfer_received+=2;
		}
		if(http_next_transfer_received>=http_next_transfer_length){
                  http_next_transfer_body=0;
		}

              }else{
                // finish
                finish=1;
		*p_recv_msg='\0';
	      }
            }

          }else{  // for Content-Length: ...
            int len=strlen(line);
	    if(len>=RECV_MSG_LINESIZE){
              err_buffer_overrun=1;
	    }

            // check the remaining length (raises err_buffer_overrun and drop the received data if overrun)
            int copy_length=len;
	    if(l_recv_msg<len+2){ // +2 for \r\n (if is_newline==1)
              err_buffer_overrun=1;
              copy_length=l_recv_msg;
            }

            strncpy(p_recv_msg,line,copy_length);
	    p_recv_msg+=copy_length;
            r_recv_msg+=len;
	    l_recv_msg-=copy_length;
            if(is_newline){
              strncpy(p_recv_msg,"\r\n",2);
              p_recv_msg+=2;
              r_recv_msg+=2;
              l_recv_msg-=2;
	    }
	    if(r_recv_msg>=http_content_length){
              finish=1;
	      *p_recv_msg='\0';
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

    if(offset>=RECV_MSG_BUFSIZE){
      err_buffer_overrun=1;
      offset=0;
    }
  }

  // printf("PATH: %s\n",http_request_path);
  // printf("METHOD: %d\n",http_method);

  free(http_recv_buffer);
  free(line);

  // processing according to the request 
  char* send_msg=(char*)malloc(SEND_MSG_BUFSIZE);
  int n_send_msg=0;
  if(strcmp(http_request_path,IEEE1888_EPR_PATH)==0){

    if(http_method==HTTP_METHOD_POST){

      // return IEEE1888 error message (if buffer overrun)
      if(err_buffer_overrun){
        // Error Handling at SOAP-level
        strcpy(http_response_code,"500 Internal Server Error");
        strcpy(http_content_type,"text/xml;charset=utf-8");
        ieee1888_soap_error_gen("The requested message is too big and failed to accept it!!",send_msg,SEND_MSG_BUFSIZE);
        n_send_msg=strlen(send_msg);

      // if successfully received, parse and process it, and generate the result.
      }else{
        int message_type;
	int err_send_buffer_overrun=0;
        ieee1888_transport* request=ieee1888_mk_transport();

        if(ieee1888_soap_parse(recv_msg, r_recv_msg, request, &message_type)!=NULL){

          // requested message parse has been successful.
          ieee1888_transport* response=NULL;
          if(message_type==IEEE1888_QUERY_RQ){
            if(__ieee1888_server_query){
              response=__ieee1888_server_query(request,NULL);
              if(response!=NULL){
                strcpy(http_response_code,"200 OK");
                strcpy(http_content_type,"text/xml;charset=utf-8");
                if(ieee1888_soap_gen(response, IEEE1888_QUERY_RS, send_msg, SEND_MSG_BUFSIZE)==IEEE1888_SOAP_GEN_ERROR_OVERFLOW){
                  err_send_buffer_overrun=1;
		}
                n_send_msg=strlen(send_msg);
		    
                // memory recycle
                ieee1888_destroy_objects((ieee1888_object*)response);
                free(response);
              }else{
                // query failed.
                strcpy(http_response_code,"500 Internal Server Error");
                strcpy(http_content_type,"text/xml;charset=utf-8");
                ieee1888_soap_error_gen("query method did not return valid IEEE1888 objects.",send_msg,SEND_MSG_BUFSIZE);
                n_send_msg=strlen(send_msg);
              }
  
            }else{
              // query method handler is not implemented (out of service)
              // --> soapFault 
              strcpy(http_response_code,"500 Internal Server Error");
              strcpy(http_content_type,"text/xml;charset=utf-8");
              ieee1888_soap_error_gen("query method is out of service.",send_msg,SEND_MSG_BUFSIZE);
              n_send_msg=strlen(send_msg);
            }
  
          }else if(message_type==IEEE1888_DATA_RQ){
            if(__ieee1888_server_data){
              response=__ieee1888_server_data(request,NULL);
              if(response!=NULL){
                strcpy(http_response_code,"200 OK");
                strcpy(http_content_type,"text/xml;charset=utf-8");
                if(ieee1888_soap_gen(response, IEEE1888_DATA_RS, send_msg, SEND_MSG_BUFSIZE)==IEEE1888_SOAP_GEN_ERROR_OVERFLOW){
                  err_send_buffer_overrun=1;
		}
                n_send_msg=strlen(send_msg);
  
                // memory recycle
                ieee1888_destroy_objects((ieee1888_object*)response);
                free(response);
  
              }else{
                // data failed.
                strcpy(http_response_code,"500 Internal Server Error");
                strcpy(http_content_type,"text/xml;charset=utf-8");
                ieee1888_soap_error_gen("data method did not return valid IEEE1888 objects.",send_msg,SEND_MSG_BUFSIZE);
                n_send_msg=strlen(send_msg);
              }
            }else{
              // data method handler is not implemented (out of service)
              // --> soapFault
              strcpy(http_response_code,"500 Internal Server Error");
              strcpy(http_content_type,"text/xml;charset=utf-8");
              ieee1888_soap_error_gen("data method is out of service.",send_msg,131072);
              n_send_msg=strlen(send_msg);
            }
  
          }else{
            // unknown message type.
            strcpy(http_response_code,"500 Internal Server Error");
            strcpy(http_content_type,"text/xml;charset=utf-8");
            ieee1888_soap_error_gen("Unknown message type was received at the SOAP server.",send_msg,SEND_MSG_BUFSIZE);
            n_send_msg=strlen(send_msg);
          }

	  if(err_send_buffer_overrun){
            // Error Handling at SOAP-level
            strcpy(http_response_code,"500 Internal Server Error");
            strcpy(http_content_type,"text/xml;charset=utf-8");
            ieee1888_soap_error_gen("The response message generated has exceeded the internal buffer size (server application code misunderstood the size of the sending buffer -- try smaller acceptableSize or consider the request)!!",send_msg,SEND_MSG_BUFSIZE);
            n_send_msg=strlen(send_msg);
	  }
	
        }else{
          // failed to parse the requested XML strings.
          //  --> soapFault
          strcpy(http_response_code,"500 Internal Server Error");
          strcpy(http_content_type,"text/xml;charset=utf-8");
          ieee1888_soap_error_gen("Invalid SOAP request.",send_msg,SEND_MSG_BUFSIZE);
          n_send_msg=strlen(send_msg);
        }

        // destroy objects (memory recycle)
        if(request!=NULL){
          ieee1888_destroy_objects((ieee1888_object*)request);
          free(request);
        }
      }

    }else{
      // for GET -- show that there is an IEEE1888 Service !!
      strcpy(http_response_code,"200 OK");
      strcpy(http_content_type,"text/html");
      strcpy(send_msg,"<h1><i>Here, there is an IEEE1888 Service!!</i></h1>");
      n_send_msg=strlen(send_msg);
    }

  }else if(strcmp(http_request_path,IEEE1888_WSDL_PATH)==0){
    strcpy(http_response_code,"200 OK");
    strcpy(http_content_type,"text/xml;charset=utf-8");
     n_send_msg=ieee1888_server_print_wsdl(send_msg,cs,IEEE1888_EPR_PATH);
  }else{
    // TODO: read from file ( .... currently return not found)
    strcpy(http_response_code,"404 NotFound");
    strcpy(http_content_type,"text/html");
    strcpy(send_msg,"<h1>404 NotFound</h1>");
    n_send_msg=strlen(send_msg);
  }

  int h=0;
  char responseHeader[400];
  h+=sprintf(&responseHeader[h],"HTTP/1.1 %s\r\n",http_response_code); 
  h+=sprintf(&responseHeader[h],"Content-Type: %s\r\n",http_content_type);
  h+=sprintf(&responseHeader[h],"Content-Length: %d\r\n",n_send_msg); 
  h+=sprintf(&responseHeader[h],"\r\n"); 
  write(cs,responseHeader,h);
  // printf(responseHeader); fflush(stdout);  // for debug
  // printf(send_msg); fflush(stdout);        // for debug
  write(cs,send_msg,n_send_msg);
  close(cs);

  free(recv_msg);
  free(send_msg);

  // detach
  pthread_detach(session->thread);
  session->ttl=0;
  
  // exit
  pthread_exit(NULL);
  return NULL;
}

// Create IEEE1888 Server Daemon
//  -- this method goes into infinitive loop for accepting TCP connections from remote host (if no failure)
//     and calls back __ieee1888_server_query or __ieee1888_server_data methods (if not null)

#define IEEE1888_SERVER_OK                    0
#define IEEE1888_SERVER_ERROR_DNS_RESOLVE     1
#define IEEE1888_SERVER_ERROR_SOCKET_CREATE   2
#define IEEE1888_SERVER_ERROR_SOCKET_LISTEN   3
#define IEEE1888_SERVER_ERROR_SOCKET_SELECT   4
#define IEEE1888_SERVER_ERROR_SOCKET_ACCEPT   5

int ieee1888_server_create(int port){

  int i,m;  
  struct s_opt sopt;
  struct addrinfo hints, *res, *res0;
  int err;
  struct sockaddr_storage ss;
  struct sockaddr* sa = (struct sockaddr *)&ss;
  socklen_t len=sizeof(ss);
  int s[FD_SETSIZE];
  int smax=0;
  int sock_max=-1;
  fd_set rfd, rfd0;
  char str_port[10];
  // char host[NI_MAXHOST];
  // char serv[NI_MAXSERV];
  int backlog=15;

  // initialize and start session management
  memset(__ieee1888_session,0,sizeof(__ieee1888_session));
  pthread_mutex_init(&__ieee1888_server_http_session_mx,0);
  pthread_create(&__ieee1888_server_http_session_mgmt,0,ieee1888_server_http_session_mgmt,0);
  
  memset(&sopt,0,sizeof(sopt));
  sopt.family= PF_UNSPEC;

  sprintf(str_port,"%d",port);
  sopt.service=str_port;

  memset(&hints,0,sizeof(hints));
  hints.ai_family=sopt.family;
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_flags=AI_PASSIVE;

  err=getaddrinfo(sopt.host, sopt.service, &hints, &res0);
  if(err){
    fprintf(stderr,"getaddrinfo: %d\n",err);
    return IEEE1888_SERVER_ERROR_DNS_RESOLVE;
  }

  for(res=res0; res && smax < FD_SETSIZE; res=res->ai_next){
    s[smax]=socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s[smax]<0){
      // fprintf(stderr,"socket() failed %d\n",s[smax]);
      continue;
    }
    if(res->ai_family==PF_LOCAL){
      if(sopt.debug){
         fprintf(stderr, "skip: ai_family=PF_LOCAL\n");
      }
      continue;
    }

    if(bind(s[smax],(struct sockaddr *)res->ai_addr, res->ai_addrlen)<0){
      // getnameinfo((struct sockaddr *)res->ai_addr, res->ai_addrlen,
      //    host, sizeof(host),
      //    serv, sizeof(serv),
      //    NI_NUMERICHOST|NI_NUMERICSERV);
      //fprintf(stderr,"bind(2) failed: [%s]:%s\n",host,serv);
      close(s[smax]);
      s[smax]=-1;
      continue;
    }else{
      // getnameinfo((struct sockaddr *)res->ai_addr, res->ai_addrlen,
      //   host, sizeof(host),
      //   serv, sizeof(serv),
      //   NI_NUMERICHOST|NI_NUMERICSERV);
      //fprintf(stderr,"bind(2) succeeded: [%s]:%s\n",host,serv);
    }
    if(listen(s[smax],backlog)<0){
      // getnameinfo((struct sockaddr *)res->ai_addr, res->ai_addrlen,
      //    host, sizeof(host),
      //    serv, sizeof(serv),
      //    NI_NUMERICHOST|NI_NUMERICSERV);
      //fprintf(stderr,"listen(2) failed: [%s]:%s\n",host,serv);
      close(s[smax]);
      s[smax]=-1;
      continue;
    }else{
      // getnameinfo((struct sockaddr *)res->ai_addr, res->ai_addrlen,
      //  host, sizeof(host),
      //  serv, sizeof(serv),
      //  NI_NUMERICHOST|NI_NUMERICSERV);
      //fprintf(stderr,"listen(2) succeeded: [%s]:%s\n",host,serv);
    }

    if(s[smax]>sock_max){
      sock_max=s[smax];
    }
    smax++;
  }

  if(smax==0){
    fprintf(stderr,"no sockets\n");
    return IEEE1888_SERVER_ERROR_SOCKET_CREATE;
  }
  freeaddrinfo(res0);

  FD_ZERO(&rfd0);
  for(i=0;i<smax;i++){
    FD_SET(s[i],&rfd0);
  }

  // tolerance for broken pipes 
  signal(SIGPIPE,SIG_IGN);

  while(1){
    rfd=rfd0;
    m=select(sock_max+1,&rfd, NULL, NULL, NULL);
    if(m<0){
      if(errno==EINTR){
        continue;
      }
      perror("select");
      return IEEE1888_SERVER_ERROR_SOCKET_SELECT;
    }
    
    for(i=0;i<smax;i++){
      if(FD_ISSET(s[i],&rfd)){
        int cs;

	// create a thread to falk into parallel processing !!!
	cs=accept(s[i],sa,&len);
	if(cs<0){
	  perror("accept");
	  return IEEE1888_SERVER_ERROR_SOCKET_ACCEPT;
	}

	// create thread (for multiple threads)
	int j;
	pthread_mutex_lock(&__ieee1888_server_http_session_mx);
	for(j=0;j<IEEE1888_SERVER_HTTP_SESSION_COUNT;j++){
	  if(__ieee1888_session[j].ttl==0){
	    break;
	  }
	}
	pthread_mutex_unlock(&__ieee1888_server_http_session_mx);

	if(j<IEEE1888_SERVER_HTTP_SESSION_COUNT){
	  __ieee1888_session[j].ttl=IEEE1888_SERVER_HTTP_SESSION_TIMEOUT;
	  __ieee1888_session[j].socket=cs;
	  
	  int create_failed=pthread_create(&(__ieee1888_session[j].thread),0,ieee1888_server_http_proc,(void*)&(__ieee1888_session[j]));
  
          if(create_failed){
            // ERROR: failed to create threads

	    close(cs);
	  }

	}else{
	  // ERROR: too many sessions -- try later
	  close(cs);
	}
      }
    }
  }
}

