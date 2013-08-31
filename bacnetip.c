/*
 * Simplified BACnet/IP Stack
 *   only supports 
 *     1. readProperty client
 *     2. writeProperty client
 *
 * author: Hideya Ochiai
 * create: 2012-06-25
 * update: 2012-12-06 from Dallas to Tokyo 
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

#include "bacnetip.h"

#define BACNET_MAX_TRY_COUNT 3
#define BACNET_PACKET_MAX_LEN 1200
#define BACNET_RESP_TIMEOUT 1

static unsigned char invokeID=1;

void bacnetip_recv_fail(int signum);

int bacnetip_invoke(const char* host, unsigned short port, 
                    const unsigned char* rq, int n_rq,
		    unsigned char* rs, int* n_rs){  
  int sock;
  struct sockaddr_in s0;
  struct sockaddr_in s1;
  int recvlen, addrlen;

  if((sock=socket(AF_INET,SOCK_DGRAM,0))<0){
    fprintf(stderr,"Socket error\n");
    return BACNET_NG;
  }

  struct timeval tm;
  tm.tv_sec=3;
  tm.tv_usec=0;
  setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&tm,sizeof(tm));

  bzero((char*)&s0,sizeof(s0));
  s0.sin_family=AF_INET;
  s0.sin_addr.s_addr=htonl(INADDR_ANY);
  s0.sin_port=htons(0); // any port should be OK
  if(bind(sock,(struct sockaddr*)&s0,sizeof(s0))<0){
    close(sock);
    fprintf(stderr,"Socket bind error\n");
    return BACNET_NG;
  }

  bzero((char*)&s1,sizeof(s1));
  s1.sin_family=AF_INET;
  s1.sin_addr.s_addr=inet_addr(host);
  s1.sin_port=htons(port);

  signal(SIGALRM,bacnetip_recv_fail);

  int retry;
  int try_count=0;
  
  do{
    retry=0;
    if(sendto(sock,rq,n_rq,0,(struct sockaddr*)&s1,sizeof(s1))==n_rq){
      alarm(BACNET_RESP_TIMEOUT);
    }else{
      alarm(0);
      close(sock);
      fprintf(stderr,"Socket sendto error\n");
      return BACNET_NG;
    }

    addrlen=sizeof(s1);
    
    if((recvlen=recvfrom(sock,rs,*n_rs,0,(struct sockaddr*)&s1,&addrlen))<0){
      if(errno=EINTR){
        retry=1;
      }else{
        alarm(0);
        close(sock);
        fprintf(stderr,"Socket recvfrom error\n");
        return BACNET_NG;
      }
    }else{
      alarm(0);
    }

  }while(retry && ++try_count<BACNET_MAX_TRY_COUNT);

  if(try_count>=BACNET_MAX_TRY_COUNT){
    close(sock);
    return BACNET_NG;
  }
  close(sock);

  *n_rs=recvlen;

  return BACNET_OK;
}

void bacnetip_recv_fail(int signum){
  // logging the error;
  // fprintf(stdout,"bacnetip_recv_fail -- response timedout\n");
}

int parseBACnetObject(const char* packet,int data_type, int data_len, struct bacnet_data* pdata){

  switch(data_type){
    case BACNET_DATATYPE_BOOLEAN:
      if(data_len==1){
        pdata->type=BACNET_DATATYPE_BOOLEAN;
        pdata->value_boolean=1;
      }else{
        pdata->type=BACNET_DATATYPE_BOOLEAN;
        pdata->value_boolean=0;
      }
      break;

    case BACNET_DATATYPE_UNSIGNED_INT:
      if(data_len==1){
        pdata->type=BACNET_DATATYPE_UNSIGNED_INT;
	pdata->value_unsigned=((unsigned int)packet[0]&0x0ff);
      }else if(data_len==2){
        pdata->type=BACNET_DATATYPE_UNSIGNED_INT;
	pdata->value_unsigned=((unsigned int)packet[0]*0x0100&0x0ff00)
                             +((unsigned int)packet[1]*0x0001&0x000ff);
      }else if(data_len==3){
        pdata->type=BACNET_DATATYPE_UNSIGNED_INT;
	pdata->value_unsigned=((unsigned int)packet[0]*0x010000&0x0ff0000)
	                     +((unsigned int)packet[1]*0x000100&0x000ff00)
	                     +((unsigned int)packet[2]*0x000001&0x00000ff);
      }else if(data_len==4){
        pdata->type=BACNET_DATATYPE_UNSIGNED_INT;
	pdata->value_unsigned=((unsigned int)packet[0]*0x01000000&0xff000000)
	                     +((unsigned int)packet[1]*0x00010000&0x00ff0000)
                             +((unsigned int)packet[2]*0x00000100&0x0000ff00)
	                     +((unsigned int)packet[3]*0x00000001&0x000000ff);
      }else{
        fprintf(stderr,"ERROR: schema mismatch.");
        fflush(stderr);
        return -1;
      }
      break;

    case BACNET_DATATYPE_SIGNED_INT:
      if(data_len==1){
        pdata->type=BACNET_DATATYPE_SIGNED_INT;
	pdata->value_signed=((int)packet[0]);
      }else if(data_len==2){
        pdata->type=BACNET_DATATYPE_SIGNED_INT;
	pdata->value_signed=((int)packet[0]<<8)
                           |((int)packet[1]&0xff);
      }else if(data_len==3){
        pdata->type=BACNET_DATATYPE_SIGNED_INT;
	pdata->value_signed=((int)packet[0]<<16)
	                   |(((int)packet[1]&0xff)<<8)
	                   |((int)packet[2]&0xff);
      }else if(data_len==4){
        pdata->type=BACNET_DATATYPE_SIGNED_INT;
	pdata->value_signed=((int)packet[0]<<24)
	                   |(((int)packet[1]&0xff)<<16)
                           |(((int)packet[2]&0xff)<<8)
	                   |((int)packet[3]&0xff);
      }else{
        fprintf(stderr,"ERROR: schema mismatch.");
        fflush(stderr);
        return -1;
      }
      break;

    case BACNET_DATATYPE_REAL:
      if(data_len!=4){
        fprintf(stderr,"ERROR: schema mismatch.");
        fflush(stderr);
        return -1;
      }
      pdata->type=BACNET_DATATYPE_REAL;
      *(unsigned long*)(&pdata->value_real)
        =((unsigned long)packet[0]*0x01000000&0xff000000)
        +((unsigned long)packet[1]*0x00010000&0x00ff0000)
        +((unsigned long)packet[2]*0x00000100&0x0000ff00)
        +((unsigned long)packet[3]*0x00000001&0x000000ff);
      break;
      
    case BACNET_DATATYPE_OCTET_STRING:
      if(data_len>1024){
        fprintf(stderr,"ERROR: too big character string.");
        fflush(stderr);
        return -1;
      }
      pdata->type=BACNET_DATATYPE_OCTET_STRING;
      memcpy(pdata->value_str,packet,data_len);
      pdata->value_unsigned=data_len;
      break;

    case BACNET_DATATYPE_CHARACTER_STRING:
      if(data_len>1024){
        fprintf(stderr,"ERROR: too big character string.");
        fflush(stderr);
        return -1;
      }
      pdata->type=BACNET_DATATYPE_CHARACTER_STRING;
      strncpy(pdata->value_str,&packet[1],data_len-1);
      pdata->value_str[data_len-1]='\0';
      break;

    case BACNET_DATATYPE_BIT_STRING:
      if(data_len>1024){
        fprintf(stderr,"ERROR: too big character string.");
        fflush(stderr);
        return -1;
      }
      int remainder=packet[0];

      {
        int i;
        for(i=0;i<data_len*8-remainder;i++){
          if(packet[(i/8)+1]&(1<<(i%8))){
            pdata->value_str[i]='1';
          }else{
            pdata->value_str[i]='0';
          }
        }
        pdata->value_str[i]='\0';
        pdata->type=BACNET_DATATYPE_BIT_STRING;
      }
      break;

    case BACNET_DATATYPE_ENUMERATED:
      if(data_len==1){
        pdata->type=BACNET_DATATYPE_ENUMERATED;
	pdata->value_enum=((unsigned int)packet[0]&0x0ff);
      }else if(data_len==2){
        pdata->type=BACNET_DATATYPE_ENUMERATED;
	pdata->value_enum=((unsigned int)packet[0]*0x0100&0x0ff00)
                         +((unsigned int)packet[1]*0x0001&0x000ff);
      }else if(data_len==3){
        pdata->type=BACNET_DATATYPE_ENUMERATED;
	pdata->value_enum=((unsigned int)packet[0]*0x010000&0x0ff0000)
	                 +((unsigned int)packet[1]*0x000100&0x000ff00)
	                 +((unsigned int)packet[2]*0x000001&0x00000ff);
      }else if(data_len==4){
        pdata->type=BACNET_DATATYPE_ENUMERATED;
	pdata->value_enum=((unsigned int)packet[0]*0x01000000&0xff000000)
	                 +((unsigned int)packet[1]*0x00010000&0x00ff0000)
                         +((unsigned int)packet[2]*0x00000100&0x0000ff00)
	                 +((unsigned int)packet[3]*0x00000001&0x000000ff);
      }else{
        fprintf(stderr,"ERROR: schema mismatch.");
        fflush(stderr);
        return -1;
      }
      break;

    case BACNET_DATATYPE_DATE:
      // Not implemented
      break;

    case BACNET_DATATYPE_TIME:
      // Not implemented
      break;

    case BACNET_DATATYPE_OBJECT_ID:
      if(data_len==1){
        pdata->type=BACNET_DATATYPE_OBJECT_ID;
	pdata->value_unsigned=((unsigned int)packet[0]&0x0ff);
      }else if(data_len==2){
        pdata->type=BACNET_DATATYPE_OBJECT_ID;
	pdata->value_unsigned=((unsigned int)packet[0]*0x0100&0x0ff00)
                             +((unsigned int)packet[1]*0x0001&0x000ff);
      }else if(data_len==3){
        pdata->type=BACNET_DATATYPE_OBJECT_ID;
	pdata->value_unsigned=((unsigned int)packet[0]*0x010000&0x0ff0000)
	                    +((unsigned int)packet[1]*0x000100&0x000ff00)
	                    +((unsigned int)packet[2]*0x000001&0x00000ff);
      }else if(data_len==4){
        pdata->type=BACNET_DATATYPE_OBJECT_ID;
	pdata->value_unsigned=((unsigned int)packet[0]*0x01000000&0xff000000)
	                     +((unsigned int)packet[1]*0x00010000&0x00ff0000)
                             +((unsigned int)packet[2]*0x00000100&0x0000ff00)
	                     +((unsigned int)packet[3]*0x00000001&0x000000ff);
      }else{
        fprintf(stderr,"ERROR: schema mismatch.");
        fflush(stderr);
        return -1;
      }
      break;

    default: 
      printf("ERROR: unsupported data type %d \n",data_type);
      return -1;
  }

  return 0;
}

int generateBACnetObject(char* packet,int* data_type, int* data_len, const struct bacnet_data* pdata){

  switch(pdata->type){
  case BACNET_DATATYPE_BOOLEAN:
      if(pdata->value_boolean){
        *data_len=1;
      }else{
        *data_len=0;
      }
      break;

  case BACNET_DATATYPE_UNSIGNED_INT:
    if(pdata->value_unsigned<0x100){
      if(*data_len<1){
        printf("ERROR: given packet footprint is too small.\n");
        return -1;
      }
      *data_len=1;
      packet[0]=(uint8_t)(pdata->value_unsigned);
      
    }else if(pdata->value_unsigned<0x10000){
      if(*data_len<2){
        printf("ERROR: given packet footprint is too small.\n");
        return -1;
      }
      *data_len=2;
      packet[0]=(uint8_t)(pdata->value_unsigned>>8);
      packet[1]=(uint8_t)(pdata->value_unsigned);

    }else if(pdata->value_unsigned<0x1000000){
      if(*data_len<3){
        printf("ERROR: given packet footprint is too small.\n");
        return -1;
      }
      *data_len=3;
      packet[0]=(uint8_t)(pdata->value_unsigned>>16);
      packet[1]=(uint8_t)(pdata->value_unsigned>>8);
      packet[2]=(uint8_t)(pdata->value_unsigned);

    }else{
      if(*data_len<4){
        printf("ERROR: given packet footprint is too small.\n");
        return -1;
      }
      *data_len=4;
      packet[0]=(uint8_t)(pdata->value_unsigned>>24);
      packet[1]=(uint8_t)(pdata->value_unsigned>>16);
      packet[2]=(uint8_t)(pdata->value_unsigned>>8);
      packet[3]=(uint8_t)(pdata->value_unsigned);
    }
    *data_type=pdata->type;
    break;

  case BACNET_DATATYPE_SIGNED_INT:
    if(-128<=pdata->value_signed && pdata->value_signed<=127){
      if(*data_len<1){
        printf("ERROR: given packet footprint is too small.\n");
        return -1;
      }
      *data_len=1;
      packet[0]=(uint8_t)(pdata->value_signed);
      
    }else if(-32768<=pdata->value_signed && pdata->value_signed<=32767){
      if(*data_len<2){
        printf("ERROR: given packet footprint is too small.\n");
        return -1;
      }
      *data_len=2;
      packet[0]=(uint8_t)(pdata->value_signed>>8);
      packet[1]=(uint8_t)(pdata->value_signed);

    }else if(-8388608<=pdata->value_signed && pdata->value_signed<=8388607){
      if(*data_len<3){
        printf("ERROR: given packet footprint is too small.\n");
        return -1;
      }
      *data_len=3;
      packet[0]=(uint8_t)(pdata->value_signed>>16);
      packet[1]=(uint8_t)(pdata->value_signed>>8);
      packet[2]=(uint8_t)(pdata->value_signed);

    }else{
      if(*data_len<4){
        printf("ERROR: given packet footprint is too small.\n");
        return -1;
      }
      *data_len=4;
      packet[0]=(uint8_t)(pdata->value_signed>>24);
      packet[1]=(uint8_t)(pdata->value_signed>>16);
      packet[2]=(uint8_t)(pdata->value_signed>>8);
      packet[3]=(uint8_t)(pdata->value_signed);
    }
    *data_type=pdata->type;
    break;

  case BACNET_DATATYPE_REAL:
    if(*data_len<4){
      printf("ERROR: given packet footprint is too small.\n");
      return -1;
    }
    *data_len=4;
    {
      unsigned long tmp=*(unsigned long*)&(pdata->value_real);
      packet[0]=(uint8_t)(tmp/0x1000000);
      packet[1]=(uint8_t)(tmp/0x10000);
      packet[2]=(uint8_t)(tmp/0x100);
      packet[3]=(uint8_t)(tmp);
    }
    *data_type=pdata->type;
    break;

  case BACNET_DATATYPE_OCTET_STRING:
    {
      int len=pdata->value_unsigned;
      if(*data_len<len){
        fprintf(stderr,"ERROR: given packet footprint is too small.\n");
        fflush(stderr);
        return -1;
      }
      memcpy(packet,pdata->value_str,len);
      *data_len=len;
    }
    break;
    
  case BACNET_DATATYPE_CHARACTER_STRING:
    {
      int len=strlen(pdata->value_str);
      if(*data_len<len-1){
        fprintf(stderr,"ERROR: given packet footprint is too small.\n");
        fflush(stderr);
        return -1;
      }
      packet[0]=0;
      memcpy(&packet[1],pdata->value_str,len);
      *data_len=len+1;
    }
    break;

  case BACNET_DATATYPE_BIT_STRING:
    {
      int len=strlen(pdata->value_str);
      if(*data_len<(len/8)-1){
        fprintf(stderr,"ERROR: given packet footprint is too small.\n");
        fflush(stderr);
        return -1;
      }
      memset(packet,0,*data_len);
      if(len%8==0){
        packet[0]=0;
      }else{
        packet[0]=8-(len%8);
      }

      int i;
      for(i=0;i<len;i++){
        if(pdata->value_str[i]=='1'){
          packet[(i/8)+1]|=(1<<(i%8));
        }else if(pdata->value_str[i]=='0'){
        
        }else{
          fprintf(stderr,"ERROR: invalid data given (unencodable).\n");
          fflush(stderr);
          return -1;  
        }
      }
    }
    break;

  case BACNET_DATATYPE_ENUMERATED:
    if(pdata->value_enum<0x100){
      if(*data_len<1){
        printf("ERROR: given packet footprint is too small.\n");
        return -1;
      }
      *data_len=1;
      packet[0]=(uint8_t)(pdata->value_enum);
      
    }else if(pdata->value_enum<0x10000){
      if(*data_len<2){
        printf("ERROR: given packet footprint is too small.\n");
        return -1;
      }
      *data_len=2;
      packet[0]=(uint8_t)(pdata->value_enum>>8);
      packet[1]=(uint8_t)(pdata->value_enum);

    }else if(pdata->value_enum<0x1000000){
      if(*data_len<3){
        printf("ERROR: given packet footprint is too small.\n");
        return -1;
      }
      *data_len=3;
      packet[0]=(uint8_t)(pdata->value_enum>>16);
      packet[1]=(uint8_t)(pdata->value_enum>>8);
      packet[2]=(uint8_t)(pdata->value_enum);

    }else{
      if(*data_len<4){
        printf("ERROR: given packet footprint is too small.\n");
        return -1;
      }
      *data_len=4;
      packet[0]=(uint8_t)(pdata->value_enum>>24);
      packet[1]=(uint8_t)(pdata->value_enum>>16);
      packet[2]=(uint8_t)(pdata->value_enum>>8);
      packet[3]=(uint8_t)(pdata->value_enum);
    }
    *data_type=pdata->type;
    break;
  
  case BACNET_DATATYPE_DATE:
    break;

  case BACNET_DATATYPE_TIME:
    break;

  default:
    printf("ERROR: unsupported data type %d\n",pdata->type);
    return -1;
  }
  return 0;
}

int readProperty(char* host, unsigned short port, 
                 unsigned long object_id, unsigned char property_id,
		 struct bacnet_data* pdata){

  unsigned char rq_packet[BACNET_PACKET_MAX_LEN];
  unsigned char rs_packet[BACNET_PACKET_MAX_LEN];

  // invoke ID
  int thisInvokeID=invokeID++;

  // generate the request packet
  rq_packet[0] =(unsigned char)0x81; // always 0x81
  rq_packet[1] =(unsigned char)0x0A; // 0x0A means unicast packet
  rq_packet[2] =(unsigned char)0x00; // rq_packet len (upper byte)
  rq_packet[3] =(unsigned char)0x11; // rq_packet len (lower byte)
  rq_packet[4] =(unsigned char)0x01; // version (always 0x01)
  rq_packet[5] =(unsigned char)0x04; // 0x04 espects reply (confirmed request)
  rq_packet[6] =(unsigned char)0x00; // always 0
  rq_packet[7] =(unsigned char)0x00; // always 0
  rq_packet[8] =(unsigned char)thisInvokeID; // invoke ID
  rq_packet[9] =(unsigned char)0x0c; // 0x0c means readProperty
  rq_packet[10]=(unsigned char)0x0c; // 0x0c means context=0 & untagged & len=4
  rq_packet[11]=(unsigned char)(object_id>>24); // object_id (31-24 bit)
  rq_packet[12]=(unsigned char)(object_id>>16); // object_id (23-16 bit)
  rq_packet[13]=(unsigned char)(object_id>>8 ); // object_id (15-08 bit) 
  rq_packet[14]=(unsigned char)(object_id    ); // object_id (07-00 bit)
  rq_packet[15]=(unsigned char)0x19; // 0x19 means context=1 & untagged & len=1
  rq_packet[16]=(unsigned char)property_id;     // property_id (e.g., 0x55)
  
  // rq_packet[17]=(unsigned char)0x29; // 0x29 means context=2 & untagged & len=1
  // rq_packet[18]=(unsigned char)0;     // index if the property is an array

  // invoke the remote bacnet object
  int n_rs_packet=BACNET_PACKET_MAX_LEN;
  if(bacnetip_invoke(host,port,rq_packet,17,rs_packet,&n_rs_packet)!=BACNET_OK){
    return BACNET_NG;
  }

  // parse and verify the response packet
  if(n_rs_packet<13){
    fprintf(stderr,"ERROR: packet length (response) too short.");
    fflush(stderr);
    return BACNET_NG;
  }

  // for debug
  // int j=0;
  // for(j=0;j<n_rs_packet;j++){
  //   printf("%02d: %02x\n",j,rs_packet[j]);
  // }
  // fflush(stdout);
  
  if( rs_packet[0]==(unsigned char)0x81 // always 0x81
     && rs_packet[1]==(unsigned char)0x0A // 0x0A means unicast packet
     && n_rs_packet==((unsigned int)rs_packet[2]*256+(unsigned int)rs_packet[3]) // length checking
     && rs_packet[4]==(unsigned char)0x01 // version (always 1)
     && rs_packet[5]==(unsigned char)0x00 // 0x00 does not expect reply
     && rs_packet[7]==(unsigned char)thisInvokeID
     && rs_packet[8]==(unsigned char)0x0c // 0x0c means readProperty
    ){

    switch(rs_packet[6]){
    case 0x30: // COMPLEX_ACK
      // object_id compare check
      if(!(    rq_packet[10]==rs_packet[9] 
            && rq_packet[11]==rs_packet[10]
            && rq_packet[12]==rs_packet[11]
            && rq_packet[13]==rs_packet[12]
            && rq_packet[14]==rs_packet[13]
	  )
       ){
        fprintf(stderr,"ERROR: object id mismatched.");
        fflush(stderr);
        return BACNET_NG;
      }
      
      // property_id compare check
      if(!(    rq_packet[15]==rs_packet[14] 
            && rq_packet[16]==rs_packet[15]
	  )
       ){
        fprintf(stderr,"ERROR: property id mismatched.");
        fflush(stderr);
        return BACNET_NG;
      }
      
      // get into context 3 (=response data)
      int index=16;
      if(rs_packet[index]==0x3e){  // tagged data
	int data_index=0;
	int data_type=0;
        int data_len=0;
        index++; // gets into the tag
	if(rs_packet[index]&0x08){
          fprintf(stderr,"ERROR: tagged in tagged is not supported");
	  fflush(stderr);
	  return BACNET_NG;
	}
	data_type=(rs_packet[index]&0x0f0)/0x10;
        data_len=rs_packet[index]&0x07;
        if(data_len<5){
          data_index=index+1;
        }else{
          data_len=rs_packet[index+1];
          if(data_len<254){
            data_index=index+2;
          }else if(data_len==254){
            data_len=(unsigned int)rs_packet[index+2]*256+(unsigned int)rs_packet[index+3];
            data_index=index+4;
          }else if(data_len==255){
            fprintf(stderr,"ERROR: too big object contained.");
            fflush(stderr);
            return BACNET_NG;
          }
        }
	// if obtained, parse it
	if(data_type!=0 && data_index!=0 && data_len!=0 || (data_type==BACNET_DATATYPE_BOOLEAN && data_len<=1)){
          if(parseBACnetObject(&rs_packet[data_index],data_type,data_len,pdata)){
            fprintf(stderr,"ERROR: parsing the object failed.");
            fflush(stderr);
            return BACNET_NG;
	  }
	}

      }else{ // untagged data (because data type is unknown -- return BACNET_NG)
        fprintf(stderr,"ERROR: parsing the object failed (untagged -- thus data type is unknown).");
        fflush(stderr);
        return BACNET_NG;
      }
      break;

    case 0x40: // SEGMENTED_ACK
      fprintf(stderr,"ERROR: this client does not support segmentation.");
      fflush(stderr);
      return BACNET_NG;

    case 0x50: // ERROR
      fprintf(stderr,"ERROR: the request was failed (the server replied error)");
      fflush(stderr);
      return BACNET_NG;

    case 0x60: // REJECT
      fprintf(stderr,"ERROR: the request was rejected.");
      fflush(stderr);
      return BACNET_NG;

    case 0x70: // ABORT
      fprintf(stderr,"ERROR: the request was aborted.");
      fflush(stderr);
      return BACNET_NG;

    default: 
      fprintf(stderr,"ERROR: unexpected pdu type (%x)",rs_packet[6]);
      fflush(stderr);
      return BACNET_NG;
    }

  }else{
    fprintf(stderr, "ERROR: unexpected packet -- bad packet.");
    fflush(stderr);
    return BACNET_NG;
  }

  return BACNET_OK;
}

int writeProperty(char* host, unsigned short port,
                 unsigned long object_id, unsigned char property_id,
		 const struct bacnet_data* pdata){

  unsigned char rq_packet[BACNET_PACKET_MAX_LEN];
  unsigned char rs_packet[BACNET_PACKET_MAX_LEN];

  // invoke ID
  int thisInvokeID=invokeID++;

  // generate the request packet
  rq_packet[0] =(unsigned char)0x81; // always 0x81
  rq_packet[1] =(unsigned char)0x0A; // 0x0A means unicast packet
  //rq_packet[2] =(unsigned char)0x00; // rq_packet len (upper byte)
  //rq_packet[3] =(unsigned char)0x11; // rq_packet len (lower byte)
  rq_packet[4] =(unsigned char)0x01; // version (always 0x01)
  rq_packet[5] =(unsigned char)0x04; // 0x04 espects reply (confirmed request)
  rq_packet[6] =(unsigned char)0x00; // always 0
  rq_packet[7] =(unsigned char)0x00; // always 0
  rq_packet[8] =(unsigned char)thisInvokeID; // invoke ID
  rq_packet[9] =(unsigned char)0x0f; // 0x0f means writeProperty
  rq_packet[10]=(unsigned char)0x0c; // 0x0c means context=0 & untagged & len=4
  rq_packet[11]=(unsigned char)(object_id>>24); // object_id (31-24 bit)
  rq_packet[12]=(unsigned char)(object_id>>16); // object_id (23-16 bit)
  rq_packet[13]=(unsigned char)(object_id>>8 ); // object_id (15-08 bit) 
  rq_packet[14]=(unsigned char)(object_id    ); // object_id (07-00 bit)
  rq_packet[15]=(unsigned char)0x19; // 0x19 means context=1 & untagged & len=1
  rq_packet[16]=(unsigned char)property_id;     // property_id (e.g., 0x55)

  int tagged_context_len=0;
  rq_packet[17]=(unsigned char)0x3e; // 0x3e tagged context begin

  unsigned int data_type=0;
  unsigned int data_len=1024;
  unsigned char data_body[1024];
  if(generateBACnetObject(data_body,&data_type,&data_len,pdata)){
    fprintf(stderr,"ERROR: failed to generateBACnetObject\n");
    return BACNET_NG;
  }
  if(data_type==0){
    fprintf(stderr,"ERROR: unknown data type\n");
    return BACNET_NG;
  }

  if(data_type==BACNET_DATATYPE_BOOLEAN){
    rq_packet[18]=(unsigned char)((data_type<<4+data_len));  // BOOLEAN valiable is on data_len
    tagged_context_len=1; // Force 1 for BACNET_DATATYPE_BOOLEAN

  }else if(data_len<5){
    int i;
    rq_packet[18]=(unsigned char)((data_type<<4)+data_len);
    for(i=0;i<data_len;i++){
      rq_packet[19+i]=data_body[i];
    }
    tagged_context_len=1+data_len;

  }else if(data_len<254){
    int i;
    rq_packet[18]=(unsigned char)((data_type<<4)+5);
    rq_packet[19]=data_len;
    for(i=0;i<data_len;i++){
      rq_packet[20+i]=data_body[i];
    }
    tagged_context_len=2+data_len;

  }else if(data_len<1024){
    int i;
    rq_packet[18]=(unsigned char)((data_type<<4)+5);
    rq_packet[19]=254;
    rq_packet[20]=data_len/256;
    rq_packet[21]=data_len&0xff;
    for(i=0;i<data_len;i++){
      rq_packet[22+i]=data_body[i];
    }
    tagged_context_len=4+data_len;
    
  }else{
    fprintf(stderr,"ERROR: too big object contained \n");
    return BACNET_NG;
  }
  rq_packet[18+tagged_context_len]=(unsigned char)0x3f; // 0x3f tagged context end
//  rq_packet[18+tagged_context_len+1]=(unsigned char)0x4e; // 0x4e means context=4 & untagged & len=1
  rq_packet[18+tagged_context_len+1]=(unsigned char)0x49;     // priority
  rq_packet[18+tagged_context_len+2]=(unsigned char)0x08;     // priority (e.g., 8)
 // rq_packet[18+tagged_context_len+4]=(unsigned char)0x4f; // 0x4f means context=4 & untagged & len=1

  int n_rq_packet=18+tagged_context_len+2+1;
  rq_packet[2] =(unsigned char)(n_rq_packet>>8); // rq_packet len (upper byte)
  rq_packet[3] =(unsigned char)n_rq_packet; // rq_packet len (lower byte)
  
  // for debug
  // {
  //   int j=0;
  //  for(j=0;j<n_rq_packet;j++){
  //    printf("send[%02d]: %02x\n",j,rq_packet[j]);
  //  }
  //  fflush(stdout);
  //}


  // invoke the remote bacnet object
  int n_rs_packet=BACNET_PACKET_MAX_LEN;
  if(bacnetip_invoke(host,port,rq_packet,n_rq_packet,rs_packet,&n_rs_packet)!=BACNET_OK){
    return BACNET_NG;
  }
  
  // for debug
  // {
  //  int j=0;
  //  for(j=0;j<n_rs_packet;j++){
  //    printf("recv[%02d]: %02x\n",j,rs_packet[j]);
  //  }
  //  fflush(stdout);
  // }

  // parse and verify the response packet
  if(n_rs_packet<9){
    fprintf(stderr,"ERROR: packet length (response) too short.");
    fflush(stderr);
    return BACNET_NG;
  }

  if( rs_packet[0]==(unsigned char)0x81 // always 0x81
     && rs_packet[1]==(unsigned char)0x0A // 0x0A means unicast packet
     && n_rs_packet==((unsigned int)rs_packet[2]*256+(unsigned int)rs_packet[3]) // length checking
     && rs_packet[4]==(unsigned char)0x01 // version (always 1)
     && rs_packet[5]==(unsigned char)0x00 // 0x00 does not expect reply
     && rs_packet[7]==(unsigned char)thisInvokeID
     && rs_packet[8]==(unsigned char)0x0f // 0x0f means readProperty
    ){

    switch(rs_packet[6]){
    case 0x20: // SIMPLE_ACK
      break; 

    case 0x30: // COMPLEX_ACK
      // object_id compare check
      if(!(    rq_packet[10]==rs_packet[9] 
            && rq_packet[11]==rs_packet[10]
            && rq_packet[12]==rs_packet[11]
            && rq_packet[13]==rs_packet[12]
            && rq_packet[14]==rs_packet[13]
	  )
       ){
        fprintf(stderr,"ERROR: object id mismatched.");
        fflush(stderr);
        return BACNET_NG;
      }
      
      // property_id compare check
      if(!(    rq_packet[15]==rs_packet[14] 
            && rq_packet[16]==rs_packet[15]
	  )
       ){
        fprintf(stderr,"ERROR: property id mismatched.");
        fflush(stderr);
        return BACNET_NG;
      }
      break;

    case 0x40: // SEGMENTED_ACK
      fprintf(stderr,"ERROR: this client does not support segmentation.");
      fflush(stderr);
      return BACNET_NG;

    case 0x50: // ERROR
      fprintf(stderr,"ERROR: the request was failed (the server replied error)");
      fflush(stderr);
      return BACNET_NG;

    case 0x60: // REJECT
      fprintf(stderr,"ERROR: the request was rejected.");
      fflush(stderr);
      return BACNET_NG;

    case 0x70: // ABORT
      fprintf(stderr,"ERROR: the request was aborted.");
      fflush(stderr);
      return BACNET_NG;

    default: 
      fprintf(stderr,"ERROR: unexpected pdu type (%x)",rs_packet[6]);
      fflush(stderr);
      return BACNET_NG;
    }

  }else{
    fprintf(stderr, "ERROR: unexpected packet -- bad packet.");
    fflush(stderr);
    return BACNET_NG;
  }

  return BACNET_OK;
}

/*

int main(int argc, char* argv[]){

   struct bacnet_data data;
   
   if(readProperty("127.0.0.1",(unsigned short)47808,0x03400104,0x55,&data)==BACNET_OK){
      switch(data.type){
      case BACNET_DATATYPE_BOOLEAN:          printf("B %d\n",data.value_boolean); break;
      case BACNET_DATATYPE_UNSIGNED_INT:     printf("U %d\n",data.value_unsigned); break;
      case BACNET_DATATYPE_SIGNED_INT:       printf("S %d\n",data.value_signed); break;
      case BACNET_DATATYPE_REAL:             printf("R %f\n",data.value_real); break;
      case BACNET_DATATYPE_CHARACTER_STRING: printf("S %s\n",data.value_str); break;
      case BACNET_DATATYPE_ENUMERATED:       printf("E %d\n",data.value_enum); break;
      case BACNET_DATATYPE_OBJECT_ID:        printf("O %d\n",data.value_unsigned); break;
      default: printf("INFO: unknown data type %d\n",data.type);
      }
   }else{
     printf("Error");
   }

   data.type=BACNET_DATATYPE_REAL;
   data.value_real=25.5;
   if(writeProperty("127.0.0.1", (unsigned short)47808,0x03400104, 0x55,&data)==BACNET_OK){
      printf("success..");
   }else{
      printf("error..");
   }

}

*/
