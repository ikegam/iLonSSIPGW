/*
 * Simplified BACnet/IP Stack
 *   only supports 
 *     1. readProperty client
 *     2. writeProperty client
 *
 * author: Hideya Ochiai
 * create: 2012-06-25
 * update: 2012-11-30
 */

#ifndef ___BACNET_IP_H_20120625___
#define ___BACNET_IP_H_20120625___

#define BACNET_DATATYPE_NULL             0
#define BACNET_DATATYPE_BOOLEAN          1
#define BACNET_DATATYPE_UNSIGNED_INT     2
#define BACNET_DATATYPE_SIGNED_INT       3
#define BACNET_DATATYPE_REAL             4
#define BACNET_DATATYPE_DOUBLE           5
#define BACNET_DATATYPE_OCTET_STRING     6
#define BACNET_DATATYPE_CHARACTER_STRING 7
#define BACNET_DATATYPE_BIT_STRING       8
#define BACNET_DATATYPE_ENUMERATED       9
#define BACNET_DATATYPE_DATE            10
#define BACNET_DATATYPE_TIME            11
#define BACNET_DATATYPE_OBJECT_ID       12


struct bacnet_data {
int type;
int value_boolean;
int value_signed;
unsigned int value_unsigned;
float value_real;
int value_enum;
char value_str[1024];
};


#define BACNET_OK   0
#define BACNET_NG   1

// Old
// #define BACNET_INVOKE_OK                0x00000
// #define BACNET_INVOKE_ERROR             0x00100

int readProperty(char* host, unsigned short port, 
                 unsigned long object_id, unsigned char property_id,
		 struct bacnet_data* pdata);

int writeProperty(char* host, unsigned short port,
                 unsigned long object_id, unsigned char property_id,
		 const struct bacnet_data* pdata);

#endif  // #ifndef ___BACNET_IP_H_20120625___
