/*
 * ieee1888.h -- header file for using IEEE1888 stack (only Component-to-Component communication)
 *
 * create: 2011-10-30
 * update: 2012-06-22
 */

#ifndef ___IEEE1888_H_20111030___
#define ___IEEE1888_H_20111030___

#include <time.h>

// IEEE1888 Data Type: Naming
typedef char                          ieee1888_uuid;
typedef char                          ieee1888_queryType;
typedef char                          ieee1888_attrNameType;
typedef char                          ieee1888_selectType;
typedef char                          ieee1888_trapType;
typedef char                          ieee1888_time;
typedef char                          ieee1888_uri;
typedef char                          ieee1888_url;
typedef struct _ieee1888_key          ieee1888_key;
typedef struct _ieee1888_OK           ieee1888_OK;
typedef struct _ieee1888_error        ieee1888_error;
typedef struct _ieee1888_query        ieee1888_query;
typedef struct _ieee1888_header       ieee1888_header;
typedef struct _ieee1888_value        ieee1888_value;
typedef struct _ieee1888_point        ieee1888_point;
typedef struct _ieee1888_pointSet     ieee1888_pointSet;
typedef struct _ieee1888_body         ieee1888_body;
typedef struct _ieee1888_transport    ieee1888_transport;
typedef struct _ieee1888_object       ieee1888_object;

// IEEE1888 Data Type: Numbering
#define IEEE1888_DATATYPE_NONE           0
#define IEEE1888_DATATYPE_KEY            1
#define IEEE1888_DATATYPE_OK             2
#define IEEE1888_DATATYPE_ERROR          3
#define IEEE1888_DATATYPE_QUERY          4
#define IEEE1888_DATATYPE_HEADER         5
#define IEEE1888_DATATYPE_VALUE          6
#define IEEE1888_DATATYPE_POINT          7
#define IEEE1888_DATATYPE_POINTSET       8
#define IEEE1888_DATATYPE_BODY           9
#define IEEE1888_DATATYPE_TRANSPORT     10

// IEEE1888 Data Type: Schema Definition
struct _ieee1888_key {
  // internal: dtype = IEEE1888_DATATYPE_KEY
  char dtype;

  // attributes
  ieee1888_uri* id;
  ieee1888_attrNameType* attrName;
  char* eq;
  char* neq;
  char* lt;
  char* gt;
  char* lteq;
  char* gteq;
  ieee1888_selectType* select;
  ieee1888_trapType* trap;
};

struct _ieee1888_OK {
  // internal: dtype = IEEE1888_DATATYPE_OK
  char dtype;
};

struct _ieee1888_error {
  // internal: dtype = IEEE1888_DATATYPE_ERROR
  char dtype;
  
  // attribute & text_content
  char* type;
  char* content;
};

struct _ieee1888_query {
  // internal: dtype = IEEE1888_DATATYPE_QUERY
  char dtype;

  // attributes
  ieee1888_uuid* id;
  ieee1888_queryType* type;
  ieee1888_uuid* cursor;
  unsigned long ttl;
  unsigned long acceptableSize;
  char* callbackData;
  char* callbackControl;
  
  // children
  ieee1888_key* key;
  int n_key;
};

struct _ieee1888_header {
  // internal: dtype = IEEE1888_DATATYPE_HEADER
  char dtype;

  // children
  ieee1888_OK* OK;
  ieee1888_error* error;
  ieee1888_query* query;
};

struct _ieee1888_value {
  // internal: dtype = IEEE1888_DATATYPE_VALUE
  char dtype;

  // attribute & text_content
  char*  time;
  char* content;
};

struct _ieee1888_point {
  // internal: dtype = IEEE1888_DATATYPE_POINT
  char dtype;

  // attribute
  ieee1888_uri* id;

  // children
  ieee1888_value* value;
  int n_value;
};

struct _ieee1888_pointSet {
  // internal: dtype = IEEE1888_DATATYPE_POINTSET
  char dtype;
  
  // attribute
  ieee1888_uri* id;

  // children
  ieee1888_pointSet* pointSet;
  ieee1888_point* point;
  int n_pointSet;
  int n_point;
};

struct _ieee1888_body {
  // internal: dtype = IEEE1888_DATATYPE_BODY
  char dtype;

  // children
  ieee1888_pointSet* pointSet;
  ieee1888_point* point;
  int n_pointSet;
  int n_point;
};

struct _ieee1888_transport {
  // internal: dtype = IEEE1888_DATATYPE_TRANSPORT
  char dtype;

  // children
  ieee1888_header* header;
  ieee1888_body* body;
};

// abstract object
struct _ieee1888_object {
  char dtype;
};

// IEEE1888 Object Creators and Destroyer
char*               ieee1888_mk_string(const char* str);
ieee1888_time*      ieee1888_mk_time_from_string(const char* time);
ieee1888_time*      ieee1888_mk_time(time_t time);
ieee1888_time*      ieee1888_mk_time_with_tz(time_t time, long timezone_offset);
ieee1888_uri*       ieee1888_mk_uri(const char* uri);
ieee1888_url*       ieee1888_mk_url(const char* url);
ieee1888_uuid*      ieee1888_mk_uuid(const char* uuid);
ieee1888_uuid*      ieee1888_mk_new_uuid();
ieee1888_queryType*   ieee1888_mk_queryType(const char* query);
ieee1888_attrNameType* ieee1888_mk_attrNameType(const char* attrName);
ieee1888_selectType*  ieee1888_mk_selectType(const char* select); 
ieee1888_trapType*  ieee1888_mk_trapType(const char* trap);
ieee1888_key*       ieee1888_mk_key();
ieee1888_key*       ieee1888_mk_key_array(int n);
ieee1888_OK*        ieee1888_mk_OK();
ieee1888_error*     ieee1888_mk_error();
ieee1888_query*     ieee1888_mk_query();
ieee1888_header*    ieee1888_mk_header();
ieee1888_value*     ieee1888_mk_value();
ieee1888_value*     ieee1888_mk_value_array(int n);
ieee1888_point*     ieee1888_mk_point();
ieee1888_point*     ieee1888_mk_point_array(int n);
ieee1888_pointSet*  ieee1888_mk_pointSet();
ieee1888_pointSet*  ieee1888_mk_pointSet_array(int n);
ieee1888_body*      ieee1888_mk_body();
ieee1888_transport* ieee1888_mk_transport();
void                ieee1888_destroy_objects(ieee1888_object* obj);
ieee1888_object* ieee1888_clone_objects(ieee1888_object* obj, int n_array);

// Utilities
ieee1888_key*       ieee1888_add_key_to_array(ieee1888_key* key_array, int n_key, ieee1888_key* add);
ieee1888_value*     ieee1888_add_value_to_array(ieee1888_value* value_array, int n_value, ieee1888_value* add);
ieee1888_point*     ieee1888_add_point_to_array(ieee1888_point* point_array, int n_point, ieee1888_point* add);
ieee1888_pointSet*  ieee1888_add_pointSet_to_array(ieee1888_pointSet* pointSet_array, int n_pointSet, ieee1888_pointSet* add);
void                ieee1888_dump_objects(ieee1888_object* obj);

// Error Generators (in Utilities)
ieee1888_error* ieee1888_mk_error_query_not_supported(const char* msg);
ieee1888_error* ieee1888_mk_error_invalid_cursor(const char* msg);
ieee1888_error* ieee1888_mk_error_point_not_found(const char* msg);
ieee1888_error* ieee1888_mk_error_forbidden(const char* msg);
ieee1888_error* ieee1888_mk_error_value_time_not_specified(const char* msg);
ieee1888_error* ieee1888_mk_error_too_big_request(const char* msg);
ieee1888_error* ieee1888_mk_error_too_many_keys(const char* msg);
ieee1888_error* ieee1888_mk_error_too_many_values(const char* msg);
ieee1888_error* ieee1888_mk_error_invalid_request(const char* msg);
ieee1888_error* ieee1888_mk_error_server_error(const char* msg);
ieee1888_error* ieee1888_mk_error_unknown_error(const char* msg);

// IEEE1888 Server
void ieee1888_set_service_handlers(
	ieee1888_transport* (*query)(ieee1888_transport* request, char** params), 
	ieee1888_transport* (*data)(ieee1888_transport* request, char** params)
	);
int ieee1888_server_create(int port);

// IEEE1888 Client
#define IEEE1888_CLIENT_OK                      0
#define IEEE1888_CLIENT_ERROR_COMM_SCHEMA       1
#define IEEE1888_CLIENT_ERROR_SOCKET            2
#define IEEE1888_CLIENT_ERROR_DNS_RESOLVE       3
#define IEEE1888_CLIENT_ERROR_HTTP              4
#define IEEE1888_CLIENT_ERROR_SOAP              5
#define IEEE1888_CLIENT_ERROR_BUFFER_OVERFLOW  11
#define IEEE1888_CLIENT_ERROR_INTERNAL         99
ieee1888_transport* ieee1888_client_query(ieee1888_transport* request, const char* soapEPR, const char** options, int* err);
ieee1888_transport* ieee1888_client_data(ieee1888_transport* request, const char* soapEPR, const char** options, int* err);

#endif /* #ifndef ___IEEE1888_H_20111030___ */
