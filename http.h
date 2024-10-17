/*
 * http.h
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */

#ifndef HTTP_H_
#define HTTP_H_

#ifdef WIN32
#pragma once
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
//#include "Map.h"
#include "WebSocket.h"

//typedef struct {
//    size_t status;
//    char* content;
//    int resp_length;
//} RESPONSE;


RESPONSE* httpRequest(const char* Uri, char* resource, char* method, MAP* headers, MAP* data, char* appData);
char* format_query_key_val(MAP* data);

static int tls_connect(TLS_SOCKET* tls_sock, const char* hostname, const char* port);
static void tls_disconnect(TLS_SOCKET* s);
static int tls_write(TLS_SOCKET* s, const void* buffer, int size);
static int tls_read(TLS_SOCKET* s, void* buffer, unsigned long size);
static int tls_handshake_http(TLS_SOCKET* s, const char* method, const char* hostname, const char* resource, MAP* headers, char* appData);

#endif /* HTTP_H_ */
