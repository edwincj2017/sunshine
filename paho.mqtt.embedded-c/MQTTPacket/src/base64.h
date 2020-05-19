/*base64.h*/  
#ifndef _BASE64_H  
#define _BASE64_H  
  
#include <stdlib.h>  
#include <string.h>  

#if !defined(DLLImport)
  #define DLLImport 
#endif
#if !defined(DLLExport)
  #define DLLExport
#endif  

DLLExport unsigned char *base64_yd_encode(unsigned char *str);  
  
DLLExport unsigned char *bae64_yd_decode(unsigned char *code);  
  
#endif

