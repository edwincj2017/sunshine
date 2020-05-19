
#ifndef MQTTUSERAPI_H_
#define MQTTUSERAPI_H_

#include "base64.h"
#include "cJSON.h"
#include <stdio.h>

#if !defined(DLLImport)
  #define DLLImport
#endif
#if !defined(DLLExport)
  #define DLLExport
#endif


//sn+clientIdentifier+Base64(model+ softwareVersion)[ 5:13]
DLLExport int gnerate_password_key(unsigned char* sn, unsigned char* client_id, unsigned char* model, unsigned char* software_version);

#endif

