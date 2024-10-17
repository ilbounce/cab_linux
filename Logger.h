/*
 * Logger.h
 *
 *  Created on: May 25, 2023
 *      Author: ilbounce
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include "Socket.h"
#include "Structs.h"

typedef struct {
	SOCKET* sock;
	size_t* run;
	LOG_QUEUE* log_queue;
} LOGGER_PARAMS;

void start_logging(LOG_QUEUE* log_queue, CONFIG* config);

void log_msg(LOG_QUEUE* log_queue, char* msg);

static THREAD start_logging_thread(void* params);

#endif /* LOGGER_H_ */
