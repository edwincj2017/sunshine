/*******************************************************************************
 * Copyright (c) 2020 YDVERSION.
 *
 * By:Edwin
 *******************************************************************************/

#include "MQTTPacket.h"
#include "StackTrace.h"
#include "MQTTUserApi.h"
#include "base64.h"
#include "cJSON.h"
#include "md5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>




unsigned int getstimeval()
{
    unsigned int us;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    us = (int)(tv.tv_usec);
    return us;
}

  

char* generate_uuid(char buf[37])
{
    srand(getstimeval());
    const char *c = "89ab";
    char *p = buf;
    int n;
  
    for( n = 0; n < 16; ++n )
    {
        int b = rand()%255;
  
        switch( n )
        {
            case 6:
                sprintf(
                    p,
                    "4%x",
                    b%15 );
                break;
            case 8:
                sprintf(
                    p,
                    "%c%x",
                    c[rand()%strlen( c )],
                    b%15 );
                break;
            default:
                sprintf(
                    p,
                    "%02x",
                    b );
                break;
        }
  
        p += 2;
  
        switch( n )
        {
            case 3:
            case 5:
            case 7:
            case 9:
                *p++ = '-';
                break;
        }
    }
  
    *p = 0;
  
    return buf;
}



//"yunding360_"+sn+Base64(model+ softwareVersion)[ 5:13]
char* generate_password_key(char* sn, char* model, char* software_version)
{
    char *ret;
    char tmp_str[64] = {0};
    char *tmp_base64 = NULL;
    char tmp_cut_base64[10] = {0};
    char md5_16[16] = {0};
    char *md5_32 = NULL;
    MD5_CTX md5;
    int i;
    
    assert((sn != NULL) && (model != NULL) && (software_version != NULL));

    strcat(tmp_str,model);
    strcat(tmp_str,software_version);
    EDWIN_INFO("tmp_str:%s\n",tmp_str);
    
    tmp_base64 = base64_yd_encode(tmp_str);
    
    EDWIN_IMPT("get tmp_base64:%s\n",tmp_base64);
    memcpy(tmp_cut_base64,&tmp_base64[4],9);
    EDWIN_EROR("tmp_cut_base64:%s\n",tmp_cut_base64);

    ret = malloc(strlen(sn) + strlen("yunding360_") + strlen(sn) + strlen(tmp_cut_base64) + 1);
    strcat(ret, sn);
    strcat(ret, "yunding360_");
    strcat(ret, sn);
    strcat(ret,tmp_cut_base64);
    EDWIN_INFO("password key:%s\n",ret);

    
	MD5Init(&md5);
	MD5Update(&md5,ret,strlen(ret));
	MD5Final(&md5,md5_16);
    md5_32 = (char*)malloc(33);
    memset(md5_32,0x0,33);
    for(i = 0; i < 16; i++){
        sprintf(&md5_32[i*2],"%02x",md5_16[i]);
    }

    if(ret){
        free(ret);
    }

    EDWIN_IMPT("md5_32:%s\n",md5_32);
    
    return md5_32;
}


char* device_register(DEV_MSG_STRU* devmsg)
{
    assert(devmsg != NULL);
    
    cJSON *root;

    root = cJSON_CreateObject();

    if(!root) {
        EDWIN_EROR("get cjson root faild!\n");
        return NULL;
    }

    cJSON_AddStringToObject(root,"sn",devmsg->sn);
    cJSON_AddStringToObject(root,"model",devmsg->model);
    cJSON_AddStringToObject(root,"mfrs",devmsg->mfrs);
    cJSON_AddNumberToObject(root,"ability",(double)devmsg->ability);
    cJSON_AddStringToObject(root,"software_version",devmsg->software_version);
    cJSON_AddStringToObject(root,"hardware_version",devmsg->hardware_version);
    cJSON_AddStringToObject(root,"cpu_model",devmsg->cpu_model);

    char *s = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return s;
    
}



void log_yd_time(int log_level)
{
    char dst[32] = "";
    struct tm p = { 0 };
    struct timeval tv = { 0 };

    gettimeofday(&tv, NULL);

    localtime_r((const time_t *) &tv.tv_sec, &p);

    snprintf(dst, 128, "%04d-%02d-%02d %02d:%02d:%02d %08ld", p.tm_year + 1900, p.tm_mon + 1, p.tm_mday, p.tm_hour, p.tm_min,
             p.tm_sec, tv.tv_usec);

    if( INFO == log_level){
        printf(COLOR_WHITE"[INFO]%s "COLOR_DEFAUT,dst);
    }else if( IMPT == log_level){
        printf(COLOR_GREEN"[IMPT]%s "COLOR_DEFAUT,dst);
    }else if( WARN == log_level){
        printf(COLOR_BROWN"[WARN]%s "COLOR_DEFAUT,dst);
    }else if( EROR == log_level){
        printf(COLOR_RED"[EROR]%s "COLOR_DEFAUT,dst);
    }else{
        printf(COLOR_WHITE"[INFO]%s "COLOR_DEFAUT,dst);
    }
}





