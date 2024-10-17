/*
 * Logger.c
 *
 *  Created on: May 25, 2023
 *      Author: ilbounce
 */


#include "Logger.h"

void start_logging(LOG_QUEUE* log_queue, CONFIG* config)
{
	const char* address = "127.0.0.1";
	const char* port = "5555";
	int iRes = 0;
	SOCKET sock = create_server_connection(&iRes, &address, &port);
	printf("LOGGER CONNECTED\n");

	LOGGER_PARAMS* params = (LOGGER_PARAMS*)malloc(sizeof(LOGGER_PARAMS));
	params->log_queue = log_queue;
	params->run = &(config->run);
	params->sock = &sock;

	//start_logging_thread(params);

	//HANDLE thread = CreateThread(NULL, 0, start_logging_thread, params, 0, NULL);
	//HANDLE thread = _beginthread(start_logging_thread, 0, (void*)params);
	pthread_t tid;
	CREATE_THREAD(start_logging_thread, (void*)params, tid);
}

static THREAD start_logging_thread(void* params)
{
	SOCKET sock = *((LOGGER_PARAMS*)params)->sock;
	LOG_QUEUE* log_queue = ((LOGGER_PARAMS*)params)->log_queue;
	size_t* run = ((LOGGER_PARAMS*)params)->run;

//	printf("%d", *run);

	free((LOGGER_PARAMS*)params);

	while (*run) {
		char* log = pop_log(log_queue);
		if (log) {
			int length = strlen(log);
			int len = snprintf(NULL, 0, "%d", length);
			char* str_len = (char*)malloc(len + 1);
			snprintf(str_len, len+1, "%d", length);
			int res = send(sock, str_len, len + 1, 0);
			if (res == SOCKET_ERROR) {
				printf("Can't send log...\n");
			}
			else{
				int res = send(sock, log, (int)strlen(log), 0);
				if (res == SOCKET_ERROR) {
					printf("Can't send log...\n");
				}
				else{
					char buf[2];
					recv(sock, buf, 2, 0);
					//printf("%s\n", buf);
				}
			}
		}
	}

	return 0;
}

void log_msg(LOG_QUEUE* log_queue, char* msg)
{

	set_log(log_queue, msg);
}
