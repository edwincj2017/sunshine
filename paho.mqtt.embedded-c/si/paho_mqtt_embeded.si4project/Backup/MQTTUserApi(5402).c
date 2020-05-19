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
 *******************************************************************************/

#include "MQTTPacket.h"
#include "StackTrace.h"
#include "base64.h"
#include "cJSON.h"
#include <string.h>
#include <assert.h>



//"yunding360_"+sn+Base64(model+ softwareVersion)[ 5:13]
unsigned char* gnerate_password_key(unsigned char* sn, unsigned char* model, unsigned char* software_version)
{
    unsigned char* ret = NULL;
    unsigned char* client_id = "yunding360_";
    unsigned char* tmp_base64;
    
    assert((sn != NULL) && (client_id != NULL) && (model != NULL) && (software_version != NULL));

    strcat(client_id, sn);

    strcat(tmp_base64,model);
    strcat(tmp_base64,software_version);
    
    
    
    return ret;
}

