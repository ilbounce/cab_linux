/*
 * http.c
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */

#include "http.h"

extern TLSLock;

char* format_query_key_val(MAP* data)
{
    char** params = map_get_entries(data);
    char* query = (char*)malloc(1);
    query[0] = '\0';
    for (register size_t i = 0; i < data->length; i++) {

        if (i > 0) {
            query = (char*)realloc(query, strlen(query) + 2);
            strcat(query, "&");
        }
        char* key = params[i];
        char* value = (char*)map_get_value(data, key);
        query = (char*)realloc(query, strlen(query) + strlen(key) + strlen(value) + 2);
        strcat(query, key);
        strcat(query, "=");
        strcat(query, value);
    }
    free(params);

    return query;
}

RESPONSE* httpRequest(const char* Uri, char* resource, char* method, MAP* headers, MAP* data, char* appData)
{
    RESPONSE* response = malloc(sizeof(RESPONSE));

    TLS_SOCKET* sock = malloc(sizeof(TLS_SOCKET));
    char serv[8];
    char hostname[100];
    char* port;

    sscanf(Uri, "%[^:]://%[^/]", serv, hostname);
    if (strcmp(serv, "https") == 0) {
        port = "443";
    }
    else port = "80";

    if (tls_connect(sock, hostname, port) != 0) {
        printf("Can't connect to %s\n", hostname);
        free(sock);
        free(response);
        return NULL;
    }

    char* query = "";

    if (data) {
        query = (char*)malloc(strlen(resource) + 2);
        strcpy(query, resource);
        strcat(query, "?");
        char* kv_query = format_query_key_val(data);
        query = (char*)realloc(query, strlen(query) + strlen(kv_query) + 1);
        strcat(query, kv_query);
        free(kv_query);
    }

    else {
        query = (char*)malloc(strlen(resource) + 1);
        strcpy(query, resource);
    }

    if (appData) {
        unsigned long content_len = strlen(appData);
        size_t len = snprintf(NULL, 0, "%lu", content_len);
        char* cont_len = (char*)malloc(len + 1);

        snprintf(cont_len, len + 1, "%lu", content_len);

        map_set(headers, "Content-Length", cont_len);
        map_set(headers, "Accept", "*/*");
        map_set(headers, "Accept-Encoding", "gzip, deflate");
    }

    if (tls_handshake_http(sock, method, hostname, query, headers, appData) != 0) {
        free(sock);
        free(query);
        free(response);
        return NULL;
    }

    free(query);

    char* HEAD = (char*)malloc(1024);
    char buf[4];
    if (tls_read(sock, buf, sizeof(buf)) <= 0) {
        printf("Can't read from %s\n", hostname);
        tls_disconnect(sock);
        free(HEAD);
        free(response);
        return NULL;
    }
    strcpy(HEAD, buf);
    HEAD[4] = '\0';
    register int i = 4;

    while (!((HEAD[i - 4] == '\r') & (HEAD[i - 3] == '\n') & (HEAD[i - 2] == '\r') & (HEAD[i - 1] == '\n'))) {
        char buf[2];
        if (tls_read(sock, buf, sizeof(buf) - 1) <= 0) {
            printf("Can't read from %s\n", hostname);
            tls_disconnect(sock);
            free(response);
            return NULL;
        }

        buf[1] = '\0';

        HEAD[i] = buf[0];
        HEAD[i + 1] = buf[1];
        i++;
    }

    char status[4];

    sscanf(HEAD, "HTTP/1.1 %s %*s", status);

    char* enter = strstr(HEAD, "Content-Length");
    register int iCL = 0;
    if (enter){
    	iCL = enter - HEAD;
    }

    unsigned long length;

    char* content_len = (char*)malloc(8);

    if (iCL) {
        i = 0;
        while (HEAD[iCL + 16 + i] != '\r') {
            content_len[i] = HEAD[iCL + 16 + i];
            i++;
        }

        content_len[i] = '\0';
        length = atoi(content_len);
    }

    else {
        char buf[2];
        register size_t i = 0;
        while (buf[0] != '\r') {
            if (tls_read(sock, buf, sizeof(buf) - 1) <= 0) {
                printf("Can't read from %s\n", hostname);
                tls_disconnect(sock);
                free(response);
                return NULL;
            }
            content_len = (char*)realloc(content_len, i + 1);
            content_len[i] = buf[0];
            i++;
        }

        content_len[i - 1] = '\0';
        length = 0;
        for (register size_t i = 0; i < strlen(content_len); i++) {
            size_t dig = 0;
            if (content_len[i] == 'f') {
                dig = 15;
            }

            else if (content_len[i] == 'e') {
                dig = 14;
            }

            else if (content_len[i] == 'd') {
                dig = 13;
            }

            else if (content_len[i] == 'c') {
                dig = 12;
            }

            else if (content_len[i] == 'b') {
                dig = 11;
            }

            else if (content_len[i] == 'a') {
                dig = 10;
            }

            else {
                char buf[2];
                buf[0] = content_len[i];
                buf[1] = '\0';

                dig = atoi(buf);
            }

            length += dig * (unsigned long)pow(16, strlen(content_len) - 1 - i);
        }

        length++;
    }

    free(content_len);
    free(HEAD);

    response->status = atoi(status);

    char* content = malloc(length + 1);

    unsigned long readed = 0;
    content[length] = '\0';
    response->resp_length = length + 1;

    while (length > 0) {
        int result = tls_read(sock, content + readed, length);
        if (result <= 0) {
            printf("Can't read from %s\n", hostname);
            tls_disconnect(sock);
            free(content);
            free(response);
            return NULL;
        }
        readed += result;
        length -= result;
    }

    response->resp_buf = content;

    tls_disconnect(sock);

    return response;
}

static int tls_handshake_http(TLS_SOCKET* s, const char* method, const char* hostname, const char* resource, MAP* headers, char* appData)
{
    char* handshake_request;
    unsigned long request_length;
    char** head_params;

    request_length = 0;
    head_params = map_get_entries(headers);

    for (register size_t i = 0; i < headers->length; i++) {
    	char* key;
    	char* value;

		key = head_params[i];
		value = (char*)map_get_value(headers, key);

		request_length += strlen(key) + strlen(value) + 4;

	}
    if (strcmp(method, "GET") == 0) {
    	request_length += strlen(hostname) + strlen(resource) + 27;
    	handshake_request = (char*)malloc(request_length);
        strcpy(handshake_request, "GET ");
    }

    else if (strcmp(method, "POST") == 0) {
    	request_length += strlen(hostname) + strlen(resource) + 28;
    	handshake_request = (char*)malloc(request_length);
        strcpy(handshake_request, "POST ");
    }

    else if (strcmp(method, "DELETE") == 0) {
    	request_length += strlen(hostname) + strlen(resource) + 30;
    	handshake_request = (char*)malloc(request_length);
        strcpy(handshake_request, "DELETE ");
    }

    else {
        printf("%s: %s\n", "Wrong method", method);
        return -1;
    }
    strcat(handshake_request, resource);
    strcat(handshake_request, " HTTP/1.1\r\nHost: ");
    strcat(handshake_request, hostname);
    strcat(handshake_request, "\r\n");

    for (register size_t i = 0; i < headers->length; i++) {
    	char* key;
    	char* value;

        key = head_params[i];
        value = (char*)map_get_value(headers, key);

        strcat(handshake_request, key);
        strcat(handshake_request, ": ");
        strcat(handshake_request, value);
        strcat(handshake_request, "\r\n");
    }

    strcat(handshake_request, "\r\n");

    if (tls_write(s, handshake_request, strlen(handshake_request)) != 0) {
        printf("Can't handshake to %s\n", hostname);
        tls_disconnect(s);
        free(head_params);
        free(handshake_request);
        return -1;
    }

    if (appData) {
        if (tls_write(s, appData, strlen(appData)) != 0) {
            printf("Can't handshake to %s\n", hostname);
            tls_disconnect(s);
            free(head_params);
            free(handshake_request);
            return -1;
        }
    }

    free(head_params);
    free(handshake_request);

    return 0;
}

static int tls_connect(TLS_SOCKET* tls_sock, const char* hostname, const char* port)
{
#ifdef _WIN32
    // initialize windows sockets
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
    {
        return -1;
    }

    char ip[50];

    struct hostent* remoteHost;

    remoteHost = gethostbyname(hostname);

    struct in_addr* addr = malloc(sizeof(struct in_addr));

    if (remoteHost->h_addrtype == AF_INET)
    {
        addr->s_addr = *(u_long*)remoteHost->h_addr_list[0];
        //printf("IP Address: %s\n", inet_ntoa(*addr));
    }

    // create TCP IPv4 socket
    DWORD dwWaitResult = WaitForSingleObject(TLSLock, INFINITE);
    tls_sock->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tls_sock->sock == INVALID_SOCKET)
    {
        WSACleanup();
        return -1;
    }
    ReleaseMutex(TLSLock);

    // connect to server
    if (!WSAConnectByNameA(tls_sock->sock, inet_ntoa(*addr), port, NULL, NULL, NULL, NULL, NULL, NULL))
    {
        closesocket(tls_sock->sock);
        WSACleanup();
        return -1;
    }

    // initialize schannel
    {
        SCHANNEL_CRED cred =
        {
            .dwVersion = SCHANNEL_CRED_VERSION,
            .dwFlags = SCH_USE_STRONG_CRYPTO          // use only strong crypto alogorithms
                     | SCH_CRED_MANUAL_CRED_VALIDATION  // automatically validate server certificate
                     | SCH_CRED_NO_DEFAULT_CREDS,     // no client certificate authentication
            .grbitEnabledProtocols = SP_PROT_TLS1_2_CLIENT,  // allow only TLS v1.2
        };

        if (AcquireCredentialsHandleA(hostname, UNISP_NAME_A, SECPKG_CRED_OUTBOUND, NULL, &cred, NULL,
            NULL, &tls_sock->handle, NULL) != SEC_E_OK)
        {
            closesocket(tls_sock->sock);
            WSACleanup();
            return -1;
        }

        tls_sock->received = tls_sock->used = tls_sock->available = 0;
        tls_sock->decrypted = NULL;

        // perform tls handshake
        // 1) call InitializeSecurityContext to create/update schannel context
        // 2) when it returns SEC_E_OK - tls handshake completed
        // 3) when it returns SEC_I_INCOMPLETE_CREDENTIALS - server requests client certificate (not supported here)
        // 4) when it returns SEC_I_CONTINUE_NEEDED - send token to server and read data
        // 5) when it returns SEC_E_INCOMPLETE_MESSAGE - need to read more data from server
        // 6) otherwise read data from server and go to step 1

        CtxtHandle* context = NULL;
        int result = 0;
        for (;;)
        {
            SecBuffer inbuffers[2] = { 0 };
            inbuffers[0].BufferType = SECBUFFER_TOKEN;
            inbuffers[0].pvBuffer = tls_sock->incoming;
            inbuffers[0].cbBuffer = tls_sock->received;
            inbuffers[1].BufferType = SECBUFFER_EMPTY;

            SecBuffer outbuffers[1] = { 0 };
            outbuffers[0].BufferType = SECBUFFER_TOKEN;

            SecBufferDesc indesc = { SECBUFFER_VERSION, ARRAYSIZE(inbuffers), inbuffers };
            SecBufferDesc outdesc = { SECBUFFER_VERSION, ARRAYSIZE(outbuffers), outbuffers };

            DWORD flags = ISC_REQ_USE_SUPPLIED_CREDS |
                ISC_REQ_ALLOCATE_MEMORY |
                ISC_REQ_CONFIDENTIALITY |
                ISC_REQ_REPLAY_DETECT |
                ISC_REQ_SEQUENCE_DETECT |
                ISC_REQ_STREAM;
            SECURITY_STATUS sec = InitializeSecurityContextA(
                &tls_sock->handle,
                context,
                context ? NULL : (SEC_CHAR*)hostname,
                flags,
                0,
                0,
                context ? &indesc : NULL,
                0,
                context ? NULL : &tls_sock->context,
                &outdesc,
                &flags,
                NULL);

            // after first call to InitializeSecurityContext context is available and should be reused for next calls
            context = &tls_sock->context;

            if (inbuffers[1].BufferType == SECBUFFER_EXTRA)
            {
                MoveMemory(tls_sock->incoming, tls_sock->incoming + (tls_sock->received - inbuffers[1].cbBuffer), inbuffers[1].cbBuffer);
                tls_sock->received = inbuffers[1].cbBuffer;
            }
            else
            {
                tls_sock->received = 0;
            }

            if (sec == SEC_E_OK)
            {
                // tls handshake completed
                break;
            }
            else if (sec == SEC_I_INCOMPLETE_CREDENTIALS)
            {
                // server asked for client certificate, not supported here
                result = -1;
                break;
            }
            else if (sec == SEC_I_CONTINUE_NEEDED)
            {
                // need to send data to server
                char* buffer = outbuffers[0].pvBuffer;
                int size = outbuffers[0].cbBuffer;

                while (size != 0)
                {
                    int d = send(tls_sock->sock, buffer, size, 0);
                    if (d <= 0)
                    {
                        break;
                    }
                    size -= d;
                    buffer += d;
                }
                FreeContextBuffer(outbuffers[0].pvBuffer);
                if (size != 0)
                {
                    // failed to fully send data to server
                    result = -1;
                    break;
                }
            }
            else if (sec != SEC_E_INCOMPLETE_MESSAGE)
            {
                if (sec == SEC_E_CERT_EXPIRED) printf("%s\n", "SEC_E_CERT_EXPIRED");
                else if (sec == SEC_E_WRONG_PRINCIPAL) printf("%s\n", "SEC_E_WRONG_PRINCIPAL");
                else if (sec == SEC_E_UNTRUSTED_ROOT) printf("%s\n", "SEC_E_UNTRUSTED_ROOT");
                else if (sec == SEC_E_ILLEGAL_MESSAGE) printf("%s\n", "SEC_E_ILLEGAL_MESSAGE");
                else if (sec == SEC_E_ALGORITHM_MISMATCH) printf("%s\n", "SEC_E_ALGORITHM_MISMATCH");
                // SEC_E_CERT_EXPIRED - certificate expired or revoked
                // SEC_E_WRONG_PRINCIPAL - bad hostname
                // SEC_E_UNTRUSTED_ROOT - cannot vertify CA chain
                // SEC_E_ILLEGAL_MESSAGE / SEC_E_ALGORITHM_MISMATCH - cannot negotiate crypto algorithms
                result = -1;
                break;
            }

            // read more data from server when possible
            if (tls_sock->received == sizeof(tls_sock->incoming))
            {
                // server is sending too much data instead of proper handshake?
                result = -1;
                break;
            }

            int r = recv(tls_sock->sock, tls_sock->incoming + tls_sock->received, sizeof(tls_sock->incoming) - tls_sock->received, 0);
            if (r == 0)
            {
                // server disconnected socket
                return 0;
            }
            else if (r < 0)
            {
                // socket error
                result = -1;
                break;
            }
            tls_sock->received += r;
        }

        if (result != 0)
        {
            DeleteSecurityContext(context);
            FreeCredentialsHandle(&tls_sock->handle);
            closesocket(tls_sock->sock);
            WSACleanup();
            return result;
        }

        QueryContextAttributes(context, SECPKG_ATTR_STREAM_SIZES, &tls_sock->sizes);

        return 0;
    }

#elif __linux__
//    struct hostent* remoteHost;
//    struct in_addr* addr = malloc(sizeof(struct in_addr));
//    struct sockaddr_in serv_addr;
//
//    remoteHost = gethostbyname(hostname);
//    if (remoteHost->h_addrtype == AF_INET)
//    {
//        addr->s_addr = *(u_long*)remoteHost->h_addr_list[0];
//    }
//    serv_addr.sin_family = AF_INET;
//    serv_addr.sin_port = htons(atoi(port));
//
//    inet_pton(AF_INET, inet_ntoa(*addr), &serv_addr.sin_addr);

    struct addrinfo* addr;
	struct sockaddr_in* serv_addr;

	addr = NULL;

	pthread_mutex_lock(&TLSLock);
	getaddrinfo(hostname, port, 0, &addr);
	pthread_mutex_unlock(&TLSLock);

	serv_addr = (struct sockaddr_in*)addr->ai_addr;

    pthread_mutex_lock(&TLSLock);
    tls_sock->sock = socket(AF_INET, SOCK_STREAM, 0);
    pthread_mutex_unlock(&TLSLock);
    if (tls_sock->sock == -1) {
    	free(addr);
        return -1;
    }

    long arg;
    struct timeval tv;
    fd_set fdset;
    if((arg = fcntl(tls_sock->sock, F_GETFL, NULL)) < 0){
    	printf("Can't get FL\n");
    	freeaddrinfo(addr);
    	return -1;
    }
    arg |= O_NONBLOCK;
    if(fcntl(tls_sock->sock, F_SETFL, arg) < 0){
    	printf("Can't set NONBLOCK\n");
    	freeaddrinfo(addr);
    	return -1;
    }

    if (connect(tls_sock->sock, (struct sockaddr*)serv_addr, addr->ai_addrlen) < 0) {
    	if (errno == EINPROGRESS){
    		do{
    			tv.tv_sec = 1;
    			tv.tv_usec = 0;
    			FD_ZERO(&fdset);
    			FD_SET(tls_sock->sock, &fdset);

    			int res = select(tls_sock->sock + 1, NULL, &fdset, NULL, &tv);
    			if (res < 0 && errno != EINTR) {
    				printf("Error connecting %d - %s\n", errno, strerror(errno));
    				close(tls_sock->sock);
    				freeaddrinfo(addr);
    				return -1;
    			}
    			else if (res > 0){
    				unsigned long lon = sizeof(int);
    				int valopt;
    				if (getsockopt(tls_sock->sock, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0){
    					printf("Error in getsockopt() %d - %s\n", errno, strerror(errno));
    					close(tls_sock->sock);
    					freeaddrinfo(addr);
    					return -1;
    				}

    				if (valopt){
    					printf("Error in delayed connection() %d - %s\n", valopt, strerror(valopt));
    					close(tls_sock->sock);
    					freeaddrinfo(addr);
    					return -1;
    				}
    				break;
    			}

    			else{
    				printf("Socket timeout... Retry\n");
    				close(tls_sock->sock);
    				freeaddrinfo(addr);
    				return -1;
    			}
    		} while(1);
    	}
    	else{
    		printf("Error connecting %d - %s\n", errno, strerror(errno));
    		close(tls_sock->sock);
    		freeaddrinfo(addr);
    		return -1;
    	}
        //close(tls_sock->sock);
    }

    freeaddrinfo(addr);

    if((arg = fcntl(tls_sock->sock, F_GETFL, NULL)) < 0){
		printf("Can't get FL\n");
		return -1;
	}
	arg &= (~O_NONBLOCK);
	if(fcntl(tls_sock->sock, F_SETFL, arg) < 0){
		printf("Can't set NONBLOCK\n");
		return -1;
	}


    SSL_library_init();
    SSL_load_error_strings();


    tls_sock->ssl_ctx = SSL_CTX_new(SSLv23_client_method());
    if (tls_sock->ssl_ctx == NULL) {
        printf("%s\n", "Can't initialize SSL Context");
        close(tls_sock->sock);
        return -1;
    }
//    SSL_CTX_set_timeout(tls_sock->ssl_ctx, 2);
    tls_sock->ssl = SSL_new(tls_sock->ssl_ctx);
    if (tls_sock->ssl == NULL) {
        printf("%s\n", "Can't initialize SSL");
        close(tls_sock->sock);
        return -1;
    }

    SSL_set_tlsext_host_name(tls_sock->ssl, hostname);

    if (!SSL_set_fd(tls_sock->ssl, tls_sock->sock)) {
        printf("%s\n", "Can't set SSL");
        close(tls_sock->sock);
        return -1;
    }

    if (SSL_connect(tls_sock->ssl) != 1) {
        printf("%s\n", "Can't connect SSL");
        close(tls_sock->sock);
        return -1;
    }

    return 0;
#endif
}

static void tls_disconnect(TLS_SOCKET* s)
{
#ifdef _WIN32
    DWORD type = SCHANNEL_SHUTDOWN;

    SecBuffer inbuffers[1];
    inbuffers[0].BufferType = SECBUFFER_TOKEN;
    inbuffers[0].pvBuffer = &type;
    inbuffers[0].cbBuffer = sizeof(type);

    SecBufferDesc indesc = { SECBUFFER_VERSION, ARRAYSIZE(inbuffers), inbuffers };
    ApplyControlToken(&s->context, &indesc);

    SecBuffer outbuffers[1];
    outbuffers[0].BufferType = SECBUFFER_TOKEN;

    SecBufferDesc outdesc = { SECBUFFER_VERSION, ARRAYSIZE(outbuffers), outbuffers };
    DWORD flags = ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONFIDENTIALITY | ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_STREAM;
    if (InitializeSecurityContextA(&s->handle, &s->context, NULL, flags, 0, 0, &outdesc, 0, NULL, &outdesc, &flags, NULL) == SEC_E_OK)
    {
        char* buffer = outbuffers[0].pvBuffer;
        int size = outbuffers[0].cbBuffer;
        while (size != 0)
        {
            int d = send(s->sock, buffer, size, 0);
            if (d <= 0)
            {
                // ignore any failures socket will be closed anyway
                break;
            }
            buffer += d;
            size -= d;
        }
        FreeContextBuffer(outbuffers[0].pvBuffer);
    }
    shutdown(s->sock, SD_BOTH);

    DeleteSecurityContext(&s->context);
    FreeCredentialsHandle(&s->handle);
    closesocket(s->sock);
    //WSACleanup();
#elif __linux__
    SSL_shutdown(s->ssl);
    SSL_free(s->ssl);
    SSL_CTX_free(s->ssl_ctx);
    shutdown(s->sock, SHUT_WR);
    close(s->sock);
#endif
    free(s);
}

static int tls_write(TLS_SOCKET* s, const void* buffer, int size)
{
#ifdef _WIN32
    while (size != 0)
    {
        int use = min(size, s->sizes.cbMaximumMessage);

        char wbuffer[TLS_MAX_PACKET_SIZE];
        assert(s->sizes.cbHeader + s->sizes.cbMaximumMessage + s->sizes.cbTrailer <= sizeof(wbuffer));

        SecBuffer buffers[3];
        buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
        buffers[0].pvBuffer = wbuffer;
        buffers[0].cbBuffer = s->sizes.cbHeader;
        buffers[1].BufferType = SECBUFFER_DATA;
        buffers[1].pvBuffer = wbuffer + s->sizes.cbHeader;
        buffers[1].cbBuffer = use;
        buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
        buffers[2].pvBuffer = wbuffer + s->sizes.cbHeader + use;
        buffers[2].cbBuffer = s->sizes.cbTrailer;

        CopyMemory(buffers[1].pvBuffer, buffer, use);

        SecBufferDesc desc = { SECBUFFER_VERSION, ARRAYSIZE(buffers), buffers };
        SECURITY_STATUS sec = EncryptMessage(&s->context, 0, &desc, 0);
        if (sec != SEC_E_OK)
        {
            // this should not happen, but just in case check it
            return -1;
        }

        int total = buffers[0].cbBuffer + buffers[1].cbBuffer + buffers[2].cbBuffer;
        int sent = 0;
        while (sent != total)
        {
            int d = send(s->sock, wbuffer + sent, total - sent, 0);
            if (d <= 0)
            {
                // error sending data to socket, or server disconnected
                return -1;
            }
            sent += d;
        }

        buffer = (char*)buffer + use;
        size -= use;
    }
#elif __linux__
    SSL_write(s->ssl, buffer, size);
#endif

    return 0;
}

static int tls_read(TLS_SOCKET* s, void* buffer, unsigned long size)
{
    int result = 0;
#ifdef _WIN32

    while (size != 0)
    {
        if (s->decrypted)
        {
            // if there is decrypted data available, then use it as much as possible
            int use = min(size, s->available);
            CopyMemory(buffer, s->decrypted, use);
            buffer = (char*)buffer + use;
            size -= use;
            result += use;

            if (use == s->available)
            {
                // all decrypted data is used, remove ciphertext from incoming buffer so next time it starts from beginning
                MoveMemory(s->incoming, s->incoming + s->used, s->received - s->used);
                s->received -= s->used;
                s->used = 0;
                s->available = 0;
                s->decrypted = NULL;
            }
            else
            {
                s->available -= use;
                s->decrypted += use;
            }
        }
        else
        {
            // if any ciphertext data available then try to decrypt it
            if (s->received != 0)
            {
                SecBuffer buffers[4];
                assert(s->sizes.cBuffers == ARRAYSIZE(buffers));

                buffers[0].BufferType = SECBUFFER_DATA;
                buffers[0].pvBuffer = s->incoming;
                buffers[0].cbBuffer = s->received;
                buffers[1].BufferType = SECBUFFER_EMPTY;
                buffers[2].BufferType = SECBUFFER_EMPTY;
                buffers[3].BufferType = SECBUFFER_EMPTY;

                SecBufferDesc desc = { SECBUFFER_VERSION, ARRAYSIZE(buffers), buffers };

                SECURITY_STATUS sec = DecryptMessage(&s->context, &desc, 0, NULL);
                if (sec == SEC_E_OK)
                {
                    assert(buffers[0].BufferType == SECBUFFER_STREAM_HEADER);
                    assert(buffers[1].BufferType == SECBUFFER_DATA);
                    assert(buffers[2].BufferType == SECBUFFER_STREAM_TRAILER);

                    s->decrypted = buffers[1].pvBuffer;
                    s->available = buffers[1].cbBuffer;
                    s->used = s->received - (buffers[3].BufferType == SECBUFFER_EXTRA ? buffers[3].cbBuffer : 0);

                    // data is now decrypted, go back to beginning of loop to copy memory to output buffer
                    continue;
                }
                else if (sec == SEC_I_CONTEXT_EXPIRED)
                {
                    // server closed TLS connection (but socket is still open)
                    s->received = 0;
                    return result;
                }
                else if (sec == SEC_I_RENEGOTIATE)
                {
                    // server wants to renegotiate TLS connection, not implemented here
                    return -1;
                }
                else if (sec != SEC_E_INCOMPLETE_MESSAGE)
                {
                    // some other schannel or TLS protocol error
                    return -1;
                }
                // otherwise sec == SEC_E_INCOMPLETE_MESSAGE which means need to read more data
            }
            // otherwise not enough data received to decrypt

            if (result != 0)
            {
                // some data is already copied to output buffer, so return that before blocking with recv
                break;
            }

            if (s->received == sizeof(s->incoming))
            {
                // server is sending too much garbage data instead of proper TLS packet
                return -1;
            }

            // wait for more ciphertext data from server
            int r = recv(s->sock, s->incoming + s->received, sizeof(s->incoming) - s->received, 0);
            if (r == 0)
            {
                // server disconnected socket
                return 0;
            }
            else if (r < 0)
            {
                // error receiving data from socket
                result = -1;
                break;
            }
            s->received += r;
        }
    }
#elif __linux__
    result = SSL_read(s->ssl, buffer, size);
#endif

    return result;
}
