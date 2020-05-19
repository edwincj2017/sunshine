
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



DLLExport unsigned char* gnerate_password_key(unsigned char* sn, unsigned char* model, unsigned char* software_version);

#endif

