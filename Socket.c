/*
 * Socket.c
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */


#include "Socket.h"

int create_server_connection(int* iRes, const char** address, const char** port)
{
#ifdef _WIN32
    WSADATA wsaData;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    *iRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (*iRes != 0) {
        printf("WSAStartup failed with error: %d\n", *iRes);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    //int p = atoi(*port);
    *iRes = getaddrinfo(*address, *port, &hints, &result);
    if (*iRes != 0) {
        printf("getaddrinfo failed with error: %d\n", *iRes);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    *iRes = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (*iRes == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    *iRes = listen(ListenSocket, 1);
    if (*iRes == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);

#elif __linux__
    int ListenSocket = -1;
    int ClientSocket = -1;

    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == -1) {
        printf("Socket creation failed");
        return 1;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(*address);
    server.sin_port = htons(atoi(*port));

    //Bind
    *iRes = bind(ListenSocket, (struct sockaddr*)&server, sizeof(server));
    /*if (bind(ListenSocket, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        //print the error message
        printf("bind failed. Error");
        return 1;
    }*/

    //Listen
    listen(ListenSocket, 5);

    //Accept and incoming connection
    //socklen_t c = sizeof(struct sockaddr_in);

    //accept connection from an incoming client
    ClientSocket = accept(ListenSocket, (struct sockaddr*)NULL, NULL);
    if (ClientSocket < 0)
    {
        printf("accept failed");
        return 1;
    }
#endif

    return ClientSocket;
}
