
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

#define COLOR_RED                  "\e[0;31m"
#define COLOR_GREEN                "\e[0;32m"
#define COLOR_BROWN                "\e[0;33m"
#define COLOR_BLUE                 "\e[0;34m"
#define COLOR_WHITE                "\e[1;37m"
#define COLOR_DEFAUT               "\033[0m"

enum Log_Level{
     INFO = 0,
     IMPT = 1,
     WARN = 2,
     EROR = 3
};

#define YD_LOG_INFO(format,...) \
{                               \
    log_time(INFO);                 \
    printf( COLOR_WHITE  __FILE__" %s %d " format COLOR_DEFAUT, __FUNCTION__,  __LINE__,  ##__VA_ARGS__);\
}

#define YD_LOG_IMPT(format,...) \
{                               \
    log_time(IMPT);                 \
    printf( COLOR_GREEN  __FILE__" %s %d " format COLOR_DEFAUT, __FUNCTION__,  __LINE__,  ##__VA_ARGS__);\
}

#define YD_LOG_WARN(format,...) \
{                               \
    log_time(WARN);                 \
    printf( COLOR_BROWN  __FILE__" %s %d " format COLOR_DEFAUT, __FUNCTION__,  __LINE__,  ##__VA_ARGS__);\
}

#define YD_LOG_EROR(format,...) \
{                               \
    log_time(EROR);                 \
    printf( COLOR_RED  __FILE__" %s %d " format COLOR_DEFAUT, __FUNCTION__,  __LINE__,  ##__VA_ARGS__);\
}




#define EDWIN_DEBUG     /* Comment this line if not need debug msg */

#ifdef EDWIN_DEBUG

#define EDWIN_INFO(args...)  YD_LOG_INFO(args)
#define EDWIN_IMPT(args...)  YD_LOG_IMPT(args)
#define EDWIN_WARN(args...)  YD_LOG_WARN(args)
#define EDWIN_EROR(args...)  YD_LOG_EROR(args)

#else

#define EDWIN_INFO(args...)  do{}while(0)
#define EDWIN_IMPT(args...)  do{}while(0)
#define EDWIN_WARN(args...)  do{}while(0)
#define EDWIN_EROR(args...)  do{}while(0)

#endif


typedef struct{
    char* sn;
    char* model;
    char* mfrs;
    char* software_version;
    char* hardware_version;
    char* cpu_model;
    unsigned long long ability;
}DEV_MSG_STRU;


/*
int toStop = 0;--
MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
int rc = 0;--
int mysock = 0;
unsigned char buf[256];
int buflen = sizeof(buf);--
int msgid = 1;
MQTTString topicString = MQTTString_initializer;
int req_qos = 0;
int len = 0;--
char *host = "192.168.9.113";
int port = 1883;
char *passwd = NULL;
char client_id[37] = {0};
char *dev_reg_js = NULL;

*/


DLLExport char* generate_uuid(char buf[37]);
DLLExport char* generate_password_key(char* sn, char* model, char* software_version);
DLLExport char* device_register(DEV_MSG_STRU* devmsg);
DLLExport void log_time(int log_level);


#endif

