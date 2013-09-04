/*
 * Simplified iLONSS Stack
 *   only supports 
 *     1. read client
 *     2. write client
 */

#ifndef ___ILONSS_IP_H___
#define ___ILONSS_IP_H___

struct ilon_data {
  char type[1024];
  char value[1024];
  int priority;
};


#define ILONSS_OK   0
#define ILONSS_NG   1

int readProperty(char* host, unsigned short port, 
                 unsigned char* name, unsigned char* type,
		 struct ilon_data* pdata);

int writeProperty(char* host, unsigned short port,
                 unsigned char* name,
		 const struct ilon_data* pdata);

#endif  // #ifndef ___BACNET_IP_H_20120625___
