/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Sergio R. Caprile - clarifications and/or documentation extension
 *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include "MQTTPacket.h"
#include "transport.h"
#include "MQTTUserApi.h"


/* This is in order to get an asynchronous signal to stop the sample,
as the code loops waiting for msgs on the subscribed topic.
Your actual code will depend on your hw and approach*/
#include <signal.h>

#define OLD_CLIENT_API    0

enum DEVICE_CAPABILITY {
    DEVICE_CAPABILITY_VIDEO     = 0x0000000000000001ULL,
    DEVICE_CAPABILITY_AUDIO     = 0x0000000000000002ULL,
    DEVICE_CAPABILITY_PTZ       = 0x0000000000000004ULL,
    DEVICE_CAPABILITY_FLOW      = 0x0000000000000008ULL,
    DEVICE_CAPABILITY_HOTSPOT   = 0x0000000000000010ULL,
    DEVICE_CAPABILITY_FACE      = 0x0000000000000020ULL,
    DEVICE_CAPABILITY_HOTSPOTV2 = 0x0000000000000040ULL,
    DEVICE_CAPABILITY_STORE     = 0x0000000000000080ULL
};


#define CLIENT_BUF_LEN (512)

typedef struct{
    char *host;
    char *dev_reg_js;
    char client_id[37];
    unsigned char rx_buf[CLIENT_BUF_LEN];
    unsigned char tx_buf[CLIENT_BUF_LEN];
    int port;
    int req_qos;
    int mysock;
    int msgid;
    MQTTPacket_connectData data;
    MQTTString topicString;
}CLIENT_STRU;



static CLIENT_STRU client_data = {NULL,NULL,{0},{0},{0},0,0,0,0,MQTTPacket_connectData_initializer,MQTTString_initializer};
static int publish_client(char *topic,char* payload);
static int toStop = 0;


static void cfinish(int sig)
{
	signal(SIGINT, NULL);
	toStop = 1;
}

static void stop_init(void)
{
	signal(SIGINT, cfinish);
	signal(SIGTERM, cfinish);
}


static void send_hearbeat_packet(int signo)
{
    int len,rc;
    static int time_ht_cnt = 0;
    char heart_buf[20] = "yd_online";
    char *dev_reg_js = NULL;
    
    len = MQTTSerialize_pingreq(heart_buf, strlen(heart_buf));

    if(client_data.mysock){

	    rc = transport_sendPacketBuffer(client_data.mysock, heart_buf, len);
    }
    
    if(++time_ht_cnt >= 2){
        time_ht_cnt = 0;

        DEV_MSG_STRU dev_reg_data;
        dev_reg_data.ability = DEVICE_CAPABILITY_VIDEO | DEVICE_CAPABILITY_AUDIO\
            | DEVICE_CAPABILITY_HOTSPOTV2 | DEVICE_CAPABILITY_STORE;
        dev_reg_data.cpu_model = "hi3516c";
        dev_reg_data.hardware_version = "hd_1.0";
        dev_reg_data.software_version = "V20180628R12784_q1008_20";
        dev_reg_data.mfrs = "yunding";
        dev_reg_data.sn = "E305B16C01380";
        dev_reg_data.model = "DX-1002H2N-QS";
        dev_reg_js = device_register(&dev_reg_data);

		memset(client_data.tx_buf,0x0,CLIENT_BUF_LEN);
        publish_client("/reboot", dev_reg_js);

        if(dev_reg_js){
            free(dev_reg_js);
        }
    }

}

static void init_sigaction(void)
{
    struct sigaction tact;
    /*信号到了要执行的任务处理函数为prompt_info*/
    tact.sa_handler = send_hearbeat_packet;
    tact.sa_flags = 0;
    /*初始化信号集*/
    sigemptyset(&tact.sa_mask);
    /*建立信号处理机制*/
    sigaction(SIGALRM, &tact, NULL);
}

static void init_time()
{
    struct itimerval value;
    /*设定执行任务的时间间隔为2秒0微秒*/
    value.it_value.tv_sec = 5;
    value.it_value.tv_usec = 0;
    /*设定初始时间计数也为2秒0微秒*/
    value.it_interval = value.it_value;
    /*设置计时器ITIMER_REAL*/
    setitimer(ITIMER_REAL, &value, NULL);
}


static int client_init(CLIENT_STRU *client_msg)
{/*1--sucessful, 0--faild */
    //assert(client_msg != NULL);
#if 0
    client_msg->host = "192.168.8.42";
#else
    client_msg->host = "192.168.9.113";
#endif
    client_msg->msgid = 1;
    client_msg->port = 1883;
    client_msg->mysock = 0;
    client_msg->data.password.cstring = generate_password_key("E305B16C01380","YUNDING360","V20180628R12784_q1008_20");
    EDWIN_INFO("client_msg->data.password.cstring:%s\n",client_msg->data.password.cstring);
    generate_uuid(client_msg->client_id);
    client_msg->req_qos = 1;
    client_msg->data.cleansession = 1;
    
    if(36 != strlen(client_msg->client_id)){
        EDWIN_EROR("Get uuid for client_id faild\n");
        return 0;
    }
    client_msg->data.clientID.cstring = "yunding360_E305B16C01380"; //client_msg->client_id;

    EDWIN_IMPT("client id---:%s\n",client_msg->data.clientID.cstring);
    
    client_msg->data.keepAliveInterval = 6;

    //data.password.cstring = "a123456";passwd;
    client_msg->data.username.cstring = "E305B16C01380";//"4CDB714CS0020";
    
    
    return 1;

}


static int client_conc_to_server(CLIENT_STRU *client_msg)
{/*1--sucessful, 0--faild */

    int ret = 1;
    int len,rc;
    //int loop_cnt = 3;
    
    do{
            client_msg->mysock = transport_open(client_msg->host, client_msg->port);

            if(client_msg->mysock < 0){
                EDWIN_EROR("Open socket faild,mysock:%u\n",client_msg->mysock);
        		ret = 0;
                break;
            }

            len = MQTTSerialize_connect(client_msg->tx_buf, CLIENT_BUF_LEN, &(client_msg->data));

            rc = transport_sendPacketBuffer(client_msg->mysock, client_msg->tx_buf, len);
            //EDWIN_INFO("transport_sendPacketBuffer result:%u,transfer len:%u\n",rc,len);
            //usleep(5*1000);
        	/* wait for connack */
            /*while(loop_cnt--)*/{
            	if (rc = MQTTPacket_read(client_msg->rx_buf, CLIENT_BUF_LEN, transport_getdata) == CONNACK)
            	{
            		unsigned char sessionPresent, connack_rc;

            		if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, client_msg->rx_buf, CLIENT_BUF_LEN) != 1 || connack_rc != 0)
            		{
                        transport_close(client_msg->mysock);
            			EDWIN_EROR("Unable to connect, return code %d\n", connack_rc);
                        ret = 0;
                        break;
            		}
            	}
                 /*else
                {
                      EDWIN_EROR("MQTTPacket_read ret:%u\n",rc);
                      transport_close(client_msg->mysock);
                      EDWIN_WARN("Not receive connect ACK.\n");
                      ret = 0;
                }*/

                usleep(50*1000);
            	
            }
    
      }while(0);

       /*if(!loop_cnt)
      {
          transport_close(mysock);
          EDWIN_WARN("Not receive connect ACK.\n");
          ret = 0;
      }*/

    
    if(client_msg->data.password.cstring){
        free(client_msg->data.password.cstring);
    }

    return ret;
    
}


static void *subscribe_client_thread(void *threadid)
{
    int len,rc;
    unsigned char dup;
    int qos;
    unsigned char retained;
    unsigned short msgid;
    int payloadlen_in;
    unsigned char* payload_in;
    MQTTString receivedTopic;
    char* topic_name[64];
    int loop_cnt = 200;

    client_data.topicString.cstring = "/reboot";

	len = MQTTSerialize_subscribe(client_data.tx_buf, CLIENT_BUF_LEN, 0, \
        client_data.msgid++, 1, &(client_data.topicString), &(client_data.req_qos));

	rc = transport_sendPacketBuffer(client_data.mysock, client_data.tx_buf, len);
    while(loop_cnt){
    	if (MQTTPacket_read(client_data.rx_buf, CLIENT_BUF_LEN, transport_getdata) == SUBACK){
    		unsigned short submsgid;
    		int subcount;
    		int granted_qos;

    		rc = MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, client_data.rx_buf, CLIENT_BUF_LEN);
    		//if (granted_qos != 0)
    		{
    			EDWIN_EROR("granted qos %d\n", granted_qos);
    			
    		}
            break;
    	}
    	else{
            EDWIN_EROR("Not receive subscribe ACK.\n");
            loop_cnt--;
            usleep(5*1000);
            if(!loop_cnt){
                //not receive ACK in 1000ms,think subscribe faild.
                return;
            }
    	}
    }
    prctl(PR_SET_NAME,("sub_thrd"));
	while (!toStop)
	{
		memset(client_data.rx_buf,0x0,CLIENT_BUF_LEN);
		if (MQTTPacket_read(client_data.rx_buf, CLIENT_BUF_LEN, transport_getdata) == PUBLISH)
		{
			rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,\
					&payload_in, &payloadlen_in, client_data.rx_buf, CLIENT_BUF_LEN);

            if(receivedTopic.lenstring.len){
               memset(topic_name,0x0,sizeof(topic_name));
               memcpy(topic_name,receivedTopic.lenstring.data,receivedTopic.lenstring.len);
            }
			
			EDWIN_IMPT("[RX]Topic:%s,msgid:%u,qos:%u,msg:%s\n",topic_name,msgid,qos,payload_in);
		}
        
        usleep(1000*50L);
	}

    len = MQTTSerialize_disconnect(client_data.tx_buf, CLIENT_BUF_LEN);
	rc = transport_sendPacketBuffer(client_data.mysock, client_data.tx_buf, len);
    transport_close(client_data.mysock);
}


static int publish_client(char *topic,char* payload)
{
    int len,rc;


    client_data.topicString.cstring = topic;
    

    if(payload){
		    len = MQTTSerialize_publish(client_data.tx_buf, CLIENT_BUF_LEN, 0, 0, 0, 123, \
                client_data.topicString, (unsigned char*)payload, strlen(payload));
		    rc = transport_sendPacketBuffer(client_data.mysock, client_data.tx_buf, len);
            EDWIN_INFO("publish rc:%u\n",rc);
		}

    return rc;
}

int main(int argc, char *argv[])
{
	//stop_init();
    init_sigaction();
    init_time();

    client_init(&client_data);
    client_conc_to_server(&client_data);

    pthread_t threads;
    pthread_create(&threads, NULL, subscribe_client_thread, (void *)0);
    pthread_exit(NULL);
    return 0;
}
