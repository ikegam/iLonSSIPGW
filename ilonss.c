/*
 * Simplified iLONSS Stack
 *   only supports 
 *     1. read client
 *     2. write client
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "ilonss.h"
#include "sparsexml.h"

#define ILONSS_MAX_TRY_COUNT 3
#define ILONSS_PACKET_MAX_LEN 100000
#define ILONSS_RESP_TIMEOUT 3

static unsigned char invokeID=1;

SXMLParser parser;

void ilonss_recv_fail(int signum) {
}

int ilonss_invoke(const char* host, unsigned short port, 
                    const unsigned char* rq, int n_rq,
		    unsigned char* rs, int* n_rs){  
  int sock;
  struct sockaddr_in s0;
  struct sockaddr_in s1;
  int recvlen, addrlen;

  if((sock=socket(AF_INET,SOCK_STREAM,0))<0){
    fprintf(stderr,"Socket error\n");
    return ILONSS_NG;
  }

  struct timeval tm;
  tm.tv_sec=3;
  tm.tv_usec=0;
  setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&tm,sizeof(tm));

  bzero((char*)&s0,sizeof(s0));
  s0.sin_family=AF_INET;
  s0.sin_addr.s_addr=htonl(INADDR_ANY);
  s0.sin_port=htons(0); // any port should be OK
  if(bind(sock, (struct sockaddr*)&s0, sizeof(struct sockaddr_in))<0){
    close(sock);
    fprintf(stderr,"Socket bind error\n");
    return ILONSS_NG;
  }

  bzero((char*)&s1,sizeof(s1));
  s1.sin_family=AF_INET;
  s1.sin_addr.s_addr=inet_addr(host);
  s1.sin_port=htons(port);

  signal(SIGALRM,ilonss_recv_fail);

  int retry;
  int try_count=0;

#ifdef __DEBUG
  printf("Connection Established\r\n");
#endif

  do{
    retry=0;
    strcpy(rs, "");
    if (connect(sock, (struct sockaddr*)&s1,
          sizeof(struct sockaddr_in)) > 0) {
      close(sock);
      retry=1;
      continue;
    }

    if( send(sock, rq, n_rq,0) == n_rq ){
#ifdef __DEBUG
      printf("Sending Complete\r\n");
#endif
      alarm(ILONSS_RESP_TIMEOUT);
    }else{
      alarm(0);
      close(sock);
      fprintf(stderr,"Socket sendto error\n");
      return ILONSS_NG;
    }

    unsigned char receive_buf[1025];

    while ((recvlen=recv(sock, receive_buf, 1024, 0)) > 0) {
      *n_rs+=recvlen;
      receive_buf[recvlen] = '\0';
      strcat(rs, receive_buf);
    }
    alarm(0);
    close(sock);

  }while(retry && ++try_count<ILONSS_MAX_TRY_COUNT);

  if(try_count>=ILONSS_MAX_TRY_COUNT){
    return ILONSS_NG;
  }

  strcpy(rs, strstr(rs, "\r\n\r\n") + 4);

  return ILONSS_OK;
}

void bacnetip_recv_fail(int signum){
  // logging the error;
  // fprintf(stdout,"bacnetip_recv_fail -- response timedout\n");
}


/*
 <?xml version="1.0" encoding="UTF-8"?><env:Envelope xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:ilon="http://wsdl.echelon.com/web_services_ns/ilon100/v4.0/message/" xmlns:env="http://schemas.xmlsoap.org/soap/envelope/"><env:Body><ilon:Read><iLonItem><Item><UCPTname>Net/LON/iLON App/Digital Output 1/nviClaValue_1</UCPTname></Item></iLonItem></ilon:Read></env:Body></env:Envelope>
 */


int readProperty(char* host, unsigned short port,
    unsigned char* name, unsigned char* type, struct ilon_data* pdata) {

  unsigned char rq_packet_fmt[ILONSS_PACKET_MAX_LEN/2] = 
    "POST /WSDL/iLON100.WSDL HTTP/1.1\r\n"
    "SOAPAction: \"http://wsdl.echelon.com/web_services_ns/ilon100/v4.0/action/Read\"\r\n"
    "Accept: */*\r\n"
    "Content-Length: %d\r\n"
    "Content-Type: text/xml;charset=UTF-8\r\n"
    "User-Agent: ikegam iLonSS-FIAP GW\r\n"
    "Host: %s\r\n"
    "\r\n";

  unsigned char rq_packet_fmt_body[ILONSS_PACKET_MAX_LEN/2] = 
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<env:Envelope xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
    " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
    " xmlns:ilon=\"http://wsdl.echelon.com/web_services_ns/ilon100/v4.0/message/\""
    " xmlns:env=\"http://schemas.xmlsoap.org/soap/envelope/\">"
    "<env:Body><ilon:Read><iLonItem><Item>"
    "<UCPTname>%s</UCPTname>"
    "</Item></iLonItem></ilon:Read></env:Body></env:Envelope>";

  unsigned char rq_packet_body[ILONSS_PACKET_MAX_LEN];
  unsigned char rq_packet[ILONSS_PACKET_MAX_LEN];
  unsigned char rs_packet[ILONSS_PACKET_MAX_LEN];

  char key[1024], value[1024], content[1024];
  int priority = -1;

  unsigned char tagParse(char *tagname) {
    static unsigned char gotvalue = 0;
    if (strcmp(tagname, "/UCPTpriority") == 0) {
      priority = atoi(content);
    }
    if (strcmp(tagname, "/UCPTvalue") == 0 &&
        strcmp(type, value) == 0 &&
        strcmp(key, "LonFormat") == 0) {
      strcpy(pdata->value, content);
      strcpy(pdata->type, value);
      gotvalue = 1;
    }
    if (gotvalue == 1 && priority > 0) {
      return SXMLParserStop;
    }
    return SXMLParserContinue;
  }

  unsigned char contentParse(char *name) {
    strcpy(content, name);
    return SXMLParserContinue;
  }

  unsigned char keyParse(char *name) {
    strcpy(key, name);
    return SXMLParserContinue;
  }
  unsigned char valueParse(char *name) {
    strcpy(value, name);
    return SXMLParserContinue;
  }

  sxml_init_parser(&parser);
  sxml_register_func(&parser, &tagParse, &contentParse, &keyParse, &valueParse);

  // invoke ID
  int thisInvokeID=invokeID++;

  // generate the request packet
  sprintf(rq_packet_body, rq_packet_fmt_body, name);
  sprintf(rq_packet, rq_packet_fmt, strlen(rq_packet_body), host);
  strcat(rq_packet, rq_packet_body);

  // invoke the remote bacnet object
  int n_rs_packet=0;
  if (ilonss_invoke(host,port,rq_packet, 
        strlen(rq_packet), rs_packet, &n_rs_packet)!=ILONSS_OK) {
    return ILONSS_NG;
  }

  // parse and verify the response packet
  if (n_rs_packet<1000) {
    fprintf(stderr,"ERROR: packet length (response) too short.");
    fflush(stderr);
    return ILONSS_NG;
  }

  unsigned char ret = sxml_run_parser(&parser, rs_packet);

  if (ret == SXMLParserInterrupted) {
    pdata->priority = priority;
  } else {
    fprintf(stderr, "ERROR: unexpected packet -- bad packet.");
    fflush(stderr);
    return ILONSS_NG;
  }

  return ILONSS_OK;
}

/*
<?xml version="1.0" encoding="UTF-8"?><env:Envelope xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:ilon="http://wsdl.echelon.com/web_services_ns/ilon100/v4.0/message/" xmlns:env="http://schemas.xmlsoap.org/soap/envelope/"><env:Body><ilon:Write><iLonItem><Item><UCPTname>Net/LON/iLON App/Digital Output 1/nviClaValue_1</UCPTname><UCPTvalue LonFormat="UCPTvalueDef">ON</UCPTvalue><UCPTpriority>255</UCPTpriority></Item></iLonItem></ilon:Write></env:Body></env:Envelope>
*/

int writeProperty(char* host, unsigned short port,
                 unsigned char* name,
		 const struct ilon_data* pdata){

  unsigned char rq_packet_fmt[ILONSS_PACKET_MAX_LEN/2] = 
    "POST /WSDL/iLON100.WSDL HTTP/1.1\r\n"
    "SOAPAction: \"http://wsdl.echelon.com/web_services_ns/ilon100/v4.0/action/Write\"\r\n"
    "Accept: */*\r\n"
    "Content-Length: %d\r\n"
    "Content-Type: text/xml;charset=UTF-8\r\n"
    "User-Agent: ikegam iLonSS-FIAP GW\r\n"
    "Host: %s\r\n"
    "\r\n";

  unsigned char rq_packet_fmt_body[ILONSS_PACKET_MAX_LEN/2] = 
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<env:Envelope xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
    " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
    " xmlns:ilon=\"http://wsdl.echelon.com/web_services_ns/ilon100/v4.0/message/\""
    " xmlns:env=\"http://schemas.xmlsoap.org/soap/envelope/\">"
    "<env:Body><ilon:Write><iLonItem><Item>"
    "<UCPTname>%s</UCPTname>"
    "<UCPTvalue LonFormat=\"%s\">%s</UCPTvalue><UCPTpriority>%d</UCPTpriority>"
    "</Item></iLonItem></ilon:Write></env:Body></env:Envelope>";

  unsigned char rq_packet_body[ILONSS_PACKET_MAX_LEN];

  unsigned char rq_packet[ILONSS_PACKET_MAX_LEN];
  unsigned char rs_packet[ILONSS_PACKET_MAX_LEN];

  char key[1024], value[1024], content[1024];

  unsigned char tagParse(char *name) {
    if (strcmp(name, "/UCPTfaultCount") == 0 &&
        strcmp("0", content) == 0) {
      return SXMLParserStop;
    }
    return SXMLParserContinue;
  }

  unsigned char contentParse(char *name) {
    strcpy(content, name);
    return SXMLParserContinue;
  }

  unsigned char keyParse(char *name) {
    strcpy(key, name);
    return SXMLParserContinue;
  }
  unsigned char valueParse(char *name) {
    strcpy(value, name);
    return SXMLParserContinue;
  }

  sxml_init_parser(&parser);
  sxml_register_func(&parser, &tagParse, &contentParse, &keyParse, &valueParse);

  // invoke ID
  int thisInvokeID=invokeID++;

  // generate the request packet
  sprintf(rq_packet_body, rq_packet_fmt_body, name, pdata->type, pdata->value, pdata->priority);
  sprintf(rq_packet, rq_packet_fmt, strlen(rq_packet_body), host);
  strcat(rq_packet, rq_packet_body);

  int n_rs_packet=0;
  if (ilonss_invoke(host,port,rq_packet, strlen(rq_packet),rs_packet,&n_rs_packet)!=ILONSS_OK) {
    return ILONSS_NG;
  }

  // parse and verify the response packet
  if (n_rs_packet<1000) {
    fprintf(stderr,"ERROR: packet length (response) too short.");
    fflush(stderr);
    return ILONSS_NG;
  }

  unsigned char ret = sxml_run_parser(&parser, rs_packet);

  if (ret != SXMLParserInterrupted) {
    fprintf(stderr, "ERROR: unexpected packet -- bad packet.");
    fflush(stderr);
    return ILONSS_NG;
  }

  return ILONSS_OK;
}


/*
int main(int argc, char* argv[]){

   struct ilon_data data;

   if(readProperty("192.168.0.7", 80, "Net/LON/iLON App/Digital Output 1/nviClaValue_1", "UCPTvalueDef", &data)==ILONSS_OK){
     printf("INFO: data type %s %s %d\n",data.type, data.value, data.priority);
   }else{
     printf("Error");
   }

   if (strcmp(data.value, "OFF") == 0) {
     strcpy(data.value, "ON");
   } else {
     strcpy(data.value, "OFF");
   }

   strcpy(data.type, "UCPTvalueDef");
   data.priority=255;

   if(writeProperty("192.168.0.7", 80, "Net/LON/iLON App/Digital Output 1/nviClaValue_1", &data)==ILONSS_OK){
      printf("success..\n");
   }else{
      printf("error..\n");
   }

}
*/
