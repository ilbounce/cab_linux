/*
 * Socket.h
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#ifdef _WIN32
#pragma once
#endif

#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#elif __linux__
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define SOCKET_ERROR -1
typedef int SOCKET;
#endif

SOCKET create_server_connection(int* iRes, const char** address, const char** port);

#endif /* SOCKET_H_ */
