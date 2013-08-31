/**
 * ieee1888_datapool.h
 *
 * author: Hideya Ochiai
 * create: 2012-12-06 from Dallas to Tokyo
 */

#ifndef ___IEEE1888_DATAPOOL_H___
#define ___IEEE1888_DATAPOOL_H___

#include "ieee1888.h"

void ieee1888_datapool_init(const char* datapool_path, const char* ieee1888_server_url,int datapool_timespan);

void ieee1888_datapool_dump(ieee1888_transport* transport);

#endif // #ifndef ___IEEE1888_DATAPOOL_H___
