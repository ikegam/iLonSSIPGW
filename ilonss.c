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

#define __DEBUG

void ilonss_recv_fail(int signum){
  // logging the error;
  // fprintf(stdout,"ilonss_recv_fail -- response timedout\n");
}

int ilonss_invoke(const char* host, unsigned short port, 
                    const unsigned char* rq, int n_rq,
		    unsigned char* rs, int* n_rs){  
  int sock;
  struct sockaddr_in s0;
  struct sockaddr_in s1;
  int recvlen;


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
    strcpy((char *)rs, "");
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
      strcat((char *)rs, (char *)receive_buf);
    }
    alarm(0);
    close(sock);

  }while(retry && ++try_count<ILONSS_MAX_TRY_COUNT);

  if(try_count>=ILONSS_MAX_TRY_COUNT){
    return ILONSS_NG;
  }

  strcpy((char *)rs, strstr((char *)rs, "\r\n\r\n") + 4);

#ifdef __DEBUG
  printf("sent %s\n\n and received %s\n", rq, rs);
#endif

  return ILONSS_OK;
}



/*
 <?xml version="1.0" encoding="UTF-8"?><env:Envelope xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:ilon="http://wsdl.echelon.com/web_services_ns/ilon100/v4.0/message/" xmlns:env="http://schemas.xmlsoap.org/soap/envelope/"><env:Body><ilon:Read><iLonItem><Item><UCPTname>Net/LON/iLON App/Digital Output 1/nviClaValue_1</UCPTname></Item></iLonItem></ilon:Read></env:Body></env:Envelope>
 */

int readProperties(char* host, unsigned short port,
    char name[][1024], char type[][1024], struct ilon_data pdata[], int n_points) {
  SXMLExplorer* explorer;

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
    "<env:Body><ilon:Read><iLonItem>"
    "%s"
    "</iLonItem></ilon:Read></env:Body></env:Envelope>";

  unsigned char rq_packet_ilonItem_fmt[1024*80] = "";

  unsigned char rq_packet_body[ILONSS_PACKET_MAX_LEN];
  unsigned char rq_packet[ILONSS_PACKET_MAX_LEN];
  unsigned char rs_packet[ILONSS_PACKET_MAX_LEN];

  char key[1024], value[1024], content[1024];

/*
 *         <Item xsi:type="Dp_Data" >
             <UCPTname>Net/LON/iLON App/VirtFb/nvoHeatCool_207</UCPTname>
                  <UCPTannotation>Dp_Out;xsi:type=&quot;LON_Dp_Cfg&quot;</UCPTannotation>
                            <UCPThidden>0</UCPThidden>
                                 <UCPTlastUpdate>2013-10-17T01:45:06.494+09:00</UCPTlastUpdate>
                                     <UCPTvalue LonFormat="#0000000000000000[0].SNVT_hvac_mode" Unit="HVAC mode names" >HVAC_AUTO</UCPTvalue>
                                       <UCPTpointStatus LonFormat="UCPTpointStatus" >AL_NUL</UCPTpointStatus>
                                   <UCPTpriority>255</UCPTpriority>
                     </Item>
 * */

  /* */
  unsigned char tagParse(char *tagname) {
    int i;
    static unsigned char in_item = 0;
    static int process_index = 0;
    static int nof_processed = 0;
    static int priority = -1;
    static char pvalue[10][1024];
    static char ptype[10][1024];
    static unsigned char nof_values=0;
    static unsigned char gotvalue=0;

    if (strcmp(tagname, "Item") == 0) {
      in_item = 1;
    } else if (strcmp(tagname, "/Item") == 0) {
      for (i=0; i<nof_values; i++) {
        if (strcmp(type[process_index], ptype[i]) == 0) {
          nof_values = i+1;
          break;
        }
      }
      strcpy(pdata[process_index].value, pvalue[nof_values-1]);
      strcpy(pdata[process_index].type, ptype[nof_values-1]);
      pdata[process_index].priority = priority;
#ifdef __DEBUG
      printf("data %s %d, %s, %s, %d\n", name[process_index], process_index, pdata[process_index].value, pdata[process_index].type, priority);
#endif
      nof_values = 0;
      gotvalue=0;
      in_item = 0;

      nof_processed++;
      if (nof_processed >= n_points) {
        nof_processed = 0;
        return SXMLExplorerStop;
      }
    }

    if (in_item != 1) {
      return SXMLExplorerContinue;
    }

    if (strcmp(tagname, "/UCPTname") == 0) {
      for (i=0; i<n_points; i++) {
        if (strcmp(name[i], content) == 0) {
          process_index = i;
        }
      }
    }

    if (strcmp(tagname, "/UCPTpriority") == 0) {
      priority = atoi(content);
    }

    if (gotvalue ==0 && strcmp(tagname, "/UCPTvalue") == 0) {
      strcpy(pvalue[nof_values], content);
      strcpy(ptype[nof_values], value);
      nof_values++;
      if (nof_values >= 10) {
        nof_values=0;
      }
    }

    return SXMLExplorerContinue;
  }

  unsigned char contentParse(char *cname) {
    strcpy(content, cname);
    return SXMLExplorerContinue;
  }

  unsigned char keyParse(char *kname) {
    strcpy(key, kname);
    return SXMLExplorerContinue;
  }
  unsigned char valueParse(char *vname) {
    if (strcmp(key, "LonFormat") ==0) {
      strcpy(value, vname);
    }
    return SXMLExplorerContinue;
  }

  explorer = sxml_make_explorer();
  sxml_register_func(explorer, &tagParse, &contentParse, &keyParse, &valueParse);

  int i = 0;
  for (i=0; i<n_points; i++) {
    sprintf((char *)(rq_packet_ilonItem_fmt+strlen((char *)rq_packet_ilonItem_fmt)), "<Item><UCPTname>%s</UCPTname></Item>\n", name[i]);
  }

  sprintf((char *)rq_packet_body, (char *)rq_packet_fmt_body, (char *)rq_packet_ilonItem_fmt);
  sprintf((char *)rq_packet, (char *)rq_packet_fmt, strlen((char *)rq_packet_body), (char *)host);
  strcat((char *)rq_packet, (char *)rq_packet_body);


  // invoke the remote bacnet object
  int n_rs_packet=0;
  if (ilonss_invoke(host,port,rq_packet,
        strlen((char *)rq_packet), rs_packet, &n_rs_packet)!=ILONSS_OK) {
    return ILONSS_NG;
  }

  // parse and verify the response packet
  if (n_rs_packet<1000) {
    fprintf(stderr,"ERROR: packet length (response) too short.");
    fflush(stderr);
    return ILONSS_NG;
  }

  unsigned char ret = sxml_run_explorer(explorer, (char *)rs_packet);
  sxml_destroy_explorer(explorer);

  if (ret == SXMLExplorerInterrupted) {

  } else {
    fprintf(stderr, "ERROR: unexpected packet -- bad packet.");
    fflush(stderr);
    return ILONSS_NG;
  }

  return ILONSS_OK;
}

int readProperty(char* host, unsigned short port,
    char* name, char* type, struct ilon_data* pdata) {
  return readProperties(host, port, (char (*)[1024])&name, (char (*)[1024])&type, pdata, 1);
}

/*
<?xml version="1.0" encoding="UTF-8"?><env:Envelope xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:ilon="http://wsdl.echelon.com/web_services_ns/ilon100/v4.0/message/" xmlns:env="http://schemas.xmlsoap.org/soap/envelope/"><env:Body><ilon:Write><iLonItem><Item><UCPTname>Net/LON/iLON App/Digital Output 1/nviClaValue_1</UCPTname><UCPTvalue LonFormat="UCPTvalueDef">ON</UCPTvalue><UCPTpriority>255</UCPTpriority></Item></iLonItem></ilon:Write></env:Body></env:Envelope>
*/

int writeProperty(char* host, unsigned short port,
                 char* name,
                 struct ilon_data* pdata){
  SXMLExplorer* explorer;

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
      return SXMLExplorerStop;
    }
    return SXMLExplorerContinue;
  }

  unsigned char contentParse(char *name) {
    strcpy(content, name);
    return SXMLExplorerContinue;
  }

  unsigned char keyParse(char *name) {
    strcpy(key, name);
    return SXMLExplorerContinue;
  }

  unsigned char valueParse(char *name) {
    strcpy(value, name);
    return SXMLExplorerContinue;
  }

  explorer = sxml_make_explorer();
  sxml_register_func(explorer, &tagParse, &contentParse, &keyParse, &valueParse);

  // generate the request packet
  sprintf((char *)rq_packet_body, (char *)rq_packet_fmt_body, (char *)name, pdata->type, pdata->value, pdata->priority);
  sprintf((char *)rq_packet, (char *)rq_packet_fmt, strlen((char *)rq_packet_body), host);
  strcat((char *)rq_packet, (char *)rq_packet_body);

  int n_rs_packet=0;
  if (ilonss_invoke(host,port,rq_packet, strlen((char *)rq_packet),rs_packet,&n_rs_packet)!=ILONSS_OK) {
    return ILONSS_NG;
  }

  // parse and verify the response packet
  if (n_rs_packet<1000) {
    fprintf(stderr,"ERROR: packet length (response) too short.");
    fflush(stderr);
    return ILONSS_NG;
  }

  unsigned char ret;
  ret = sxml_run_explorer(explorer, (char *)rs_packet);
  sxml_destroy_explorer(explorer);

  if (ret == SXMLExplorerInterrupted) {
  } else {
    fprintf(stderr, "ERROR: unexpected packet -- bad packet.");
    fflush(stderr);
    return ILONSS_NG;
  }

  return ILONSS_OK;
}

/*
int main(int argc, char* argv[]){

   struct ilon_data data;

   if(readProperty("192.168.0.4", 80, "Net/LON/iLON App/Digital Output 1/nvoClaValueFb_1", "UCPTvalueDef", &data)==ILONSS_OK){
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

   if(writeProperty("192.168.0.4", 80, "Net/LON/iLON App/Digital Output 1/nviClaValue_1", &data)==ILONSS_OK){
      printf("success..\n");
   }else{
      printf("error..\n");
   }

}
*/
