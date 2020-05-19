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
//#include <unistd.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include "MQTTPacket.h"
#include "transport.h"
#include "MQTTUserApi.h"


/* This is in order to get an asynchronous signal to stop the sample,
as the code loops waiting for msgs on the subscribed topic.
Your actual code will depend on your hw and approach*/
#include <signal.h>


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


static CLIENT_STRU client_data;

int toStop = 0;

//MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
//int rc = 0;
//int mysock = 0;
//unsigned char buf[256];
//int buflen = sizeof(buf);
//int msgid = 1;
//MQTTString topicString = MQTTString_initializer;
//int req_qos = 0;
//int len = 0;
//char *host = "192.168.9.113";
//int port = 1883;
//char *passwd = NULL;
//char client_id[37] = {0};
//char *dev_reg_js = NULL;


void cfinish(int sig)
{
	signal(SIGINT, NULL);
	toStop = 1;
}

void stop_init(void)
{
	signal(SIGINT, cfinish);
	signal(SIGTERM, cfinish);
}


void send_hearbeat_packet(int signo)
{
    int len,rc;
    char heart_buf[20] = "yd_online";
    
    len = MQTTSerialize_pingreq(heart_buf, strlen(heart_buf));
	rc = transport_sendPacketBuffer(client_data.mysock, heart_buf, len);
    if(len != rc){
        EDWIN_EROR("Send the heartbeat packet faild.\n")
    }else{
        EDWIN_WARN("Send the heartbeat packet sucessful.\n");
    }
        
}

void init_sigaction(void)
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

void init_time()
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


static int client_init(CLIENT_STRU* client_msg)
{/*1--sucessful, 0--faild */
    //assert(client_msg != NULL);

    client_msg->host = "192.168.8.42";
    client_msg->msgid = 0;
    client_msg->port = 1883;
    client_msg->mysock = 0;
    client_msg->passwd = generate_password_key("4CDB714CS0020","DX-1002H2N-QS","V20200330R4970_f1106_20");
    generate_uuid(client_msg->client_id);
    client_msg->req_qos = 0;
    client_msg->data.cleansession = 1;
    
    if(36 != strlen(client_msg->client_id)){
        EDWIN_EROR("Get uuid for client_id faild\n");
        return 0;
    }
    client_msg->data.clientID.cstring = client_msg->client_id;
    
    client_msg->data.keepAliveInterval = 10*60;

    if(!client_msg->passwd){
        EDWIN_EROR("Get password faild\n");
        return 0;
        
    }
    
    client_msg->data.password.cstring = client_msg->passwd;
    
    return 1;

}


static int client_conc_to_server(CLIENT_STRU* client_msg)
{/*1--sucessful, 0--faild */

    int ret = 1;
    int len;
    //int buflen = sizeof(client_msg->buf);
    
    do{
            client_msg->mysock = transport_open(client_msg->host, client_msg->port);

            if(client_msg->mysock < 0){
                EDWIN_EROR("Open socket faild,mysock:%u\n",client_msg->mysock);
        		ret = 0;
                break;
            }

            len = MQTTSerialize_connect(client_msg->buf, CLIENT_BUF_LEN, &(client_msg->data));

            transport_sendPacketBuffer(client_msg->mysock, client_msg->buf, len);

        	/* wait for connack */
        	if (MQTTPacket_read(client_msg->buf, CLIENT_BUF_LEN, transport_getdata) == CONNACK)
        	{
        		unsigned char sessionPresent, connack_rc;

        		if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, client_msg->buf, CLIENT_BUF_LEN) != 1 || connack_rc != 0)
        		{
                    transport_close(client_msg->mysock);
        			EDWIN_EROR("Unable to connect, return code %d\n", connack_rc);
                    ret = 0;
                    break;
        		}
        	}
        	else{
        		//goto exit;
        		transport_close(client_msg->mysock);
        		EDWIN_WARN("Not receive connect ACK.\n");
                ret = 0;
        	}
    
      }while(0);

    
    if(client_msg->data.password.cstring){
        free(client_msg->data.password.cstring);

    return ret;
    }
    
}


static void *subClient(void *threadid)
{
    int len,rc;
    unsigned char dup;
    int qos;
    unsigned char retained;
    unsigned short msgid;
    int payloadlen_in;
    unsigned char* payload_in;
    MQTTString receivedTopic;

    client_data.topicString.cstring = "/test";

	len = MQTTSerialize_subscribe(client_data.buf, CLIENT_BUF_LEN, 0, \
        client_data.msgid++, 1, &(client_data.topicString), &(client_data.req_qos));

	rc = transport_sendPacketBuffer(client_data.mysock, client_data.buf, len);
    
	if (MQTTPacket_read(client_data.buf, CLIENT_BUF_LEN, transport_getdata) == SUBACK){
		unsigned short submsgid;
		int subcount;
		int granted_qos;

		rc = MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, client_data.buf, CLIENT_BUF_LEN);
		/*if (granted_qos != 0)
		{
			EDWIN_EROR("granted qos != 0, %d\n", granted_qos);
			goto exit;
		}*/ //0423 dtt
	}
	else{
        EDWIN_EROR("Not receive subscribe ACK.\n");
        return;
	}
    prctl(PR_SET_NAME,("sub_thrd"));
	while (1)
	{
		/* transport_getdata() has a built-in 1 second timeout,
		your mileage will vary */
		if (MQTTPacket_read(client_data.buf, CLIENT_BUF_LEN, transport_getdata) == PUBLISH)
		{
			rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,\
					&payload_in, &payloadlen_in, client_data.buf, CLIENT_BUF_LEN);
			EDWIN_INFO("message arrived %.*s\n", payloadlen_in, payload_in);
		}
        
        usleep(1000*50);
	}
}

int main(int argc, char *argv[])
{
#if 1
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	int rc = 0;
	int mysock = 0;
	unsigned char buf[200];
	int buflen = sizeof(buf);
	int msgid = 1;
	MQTTString topicString = MQTTString_initializer;
	int req_qos = 0;
	//char* payload = "mypayload";
    char* payload = "I am from embedded sdk...";
	int payloadlen = strlen(payload);
	int len = 0;
	//char *host = "m2m.eclipse.org";
	//char *host = "192.168.8.42";
    char *host = "192.168.8.42";
	int port = 1883;
    char *passwd = NULL;
    char client_id[37] = {0};
    char *dev_reg_js = NULL;
    stop_init();
#endif

	
    init_sigaction();
    init_time();
#if 0

    //CLIENT_STRU client_data;
    client_init(&client_data);
    client_conc_to_server(&client_data);

    pthread_t threads;
    pthread_create(&threads, NULL, subClient, (void *)0);
    pthread_exit(NULL);

    
#else
	if (argc > 1)
		host = argv[1];

	if (argc > 2)
		port = atoi(argv[2]);

	mysock = transport_open(host, port);
	if(mysock < 0)
		return mysock;

	EDWIN_INFO("Sending to hostname: %s port: %d\n", host, port);

    generate_uuid(client_id);

    if(36 == strlen(client_id)){
        EDWIN_IMPT("Got client_id:%s\n",client_id);
        data.clientID.cstring = client_id;
    }else{
        EDWIN_EROR("Got client_id faild.\n");
    }
    
	//data.clientID.cstring = "me";
	data.keepAliveInterval = 6; //20
	data.cleansession = 1;
	data.username.cstring = "4CDB714CS0020";
    
	
    passwd = generate_password_key("4CDB714CS0020","DX-1002H2N-QS","V20200330R4970_f1106_20");
    
    if(!passwd){
        EDWIN_EROR("Get passwd faild\n");
        
    }else{
        data.password.cstring = passwd;
        EDWIN_IMPT("Get data.password.cstring ok:%s\n",data.password.cstring);
    }
        
    //////////////////////////////conc

	len = MQTTSerialize_connect(buf, buflen, &data);
    
    if(data.password.cstring){//Added by edwin
        free(data.password.cstring);
    }

    rc = transport_sendPacketBuffer(mysock, buf, len);

	/* wait for connack */
	if (MQTTPacket_read(buf, buflen, transport_getdata) == CONNACK)
	{
		unsigned char sessionPresent, connack_rc;

		if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, buf, buflen) != 1 || connack_rc != 0)
		{
			EDWIN_EROR("Unable to connect, return code %d\n", connack_rc);
			goto exit;
		}
	}
	else
		goto exit;

	/* subscribe */
	//topicString.cstring = "substopic";
	//topicString.cstring = "iot_yd";
	//--------------------------------------subscribe
	topicString.cstring = "/test";
	len = MQTTSerialize_subscribe(buf, buflen, 0, msgid, 1, &topicString, &req_qos);

	rc = transport_sendPacketBuffer(mysock, buf, len);
	if (MQTTPacket_read(buf, buflen, transport_getdata) == SUBACK) 	/* wait for suback */
	{
		unsigned short submsgid;
		int subcount;
		int granted_qos;

		rc = MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, buf, buflen);
		/*if (granted_qos != 0)
		{
			EDWIN_EROR("granted qos != 0, %d\n", granted_qos);
			goto exit;
		}*/ //0423 dtt
	}
	else
		goto exit;

	/* loop getting msgs on subscribed topic */
	//topicString.cstring = "yd_dev_register";
	topicString.cstring = "/test";
    DEV_MSG_STRU dev_reg_data;
    dev_reg_data.ability = DEVICE_CAPABILITY_VIDEO | DEVICE_CAPABILITY_AUDIO\
        | DEVICE_CAPABILITY_HOTSPOTV2 | DEVICE_CAPABILITY_STORE;
    dev_reg_data.cpu_model = "hi3516c";
    dev_reg_data.hardware_version = "hd_1.0";
    dev_reg_data.software_version = "V20200422R5238_f1106_20";
    dev_reg_data.mfrs = "yunding";
    dev_reg_data.sn = "4CDB714CS0020";
    dev_reg_data.model = "DX-1002H2N-QS";
    dev_reg_js = device_register(&dev_reg_data);
    
    
    
	while (!toStop)
	{
		/* transport_getdata() has a built-in 1 second timeout,
		your mileage will vary */
		if (MQTTPacket_read(buf, buflen, transport_getdata) == PUBLISH)
		{
			unsigned char dup;
			int qos;
			unsigned char retained;
			unsigned short msgid;
			int payloadlen_in;
			unsigned char* payload_in;
			int rc;
			MQTTString receivedTopic;

			rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,
					&payload_in, &payloadlen_in, buf, buflen);
			EDWIN_INFO("message arrived %.*s\n", payloadlen_in, payload_in);
		}

		//printf("publishing reading\n");
		//len = MQTTSerialize_publish(buf, buflen, 0, 0, 0, 0, topicString, (unsigned char*)payload, payloadlen);
		if(dev_reg_js){
		    len = MQTTSerialize_publish(buf, buflen, 0, 0, 0, 0, topicString, (unsigned char*)dev_reg_js, strlen(dev_reg_js));
		    rc = transport_sendPacketBuffer(mysock, buf, len);
            //free(dev_reg_js);
		}
        system("sleep 10");
	}

	//printf("disconnecting\n");
	len = MQTTSerialize_disconnect(buf, buflen);
	rc = transport_sendPacketBuffer(mysock, buf, len);

exit:

    if(dev_reg_js){
        free(dev_reg_js);
    }
        
	transport_close(mysock);
#endif

	return 0;
}
