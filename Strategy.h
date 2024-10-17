/*
 * Strategy.h
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */

#ifndef STRATEGY_H_
#define STRATEGY_H_

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Socket.h"
#include "Logger.h"
//#include "Structs.h"
#include "Binance.h"
//#include "WebSocket.h"
#include "Huobi.h"
#include "Bitfinex.h"
#include "Detector.h"

//#ifdef WIN32
//#define MUTEX void*
//#elif __linux__
//#define MUTEX pthread_mutex_t
//#endif


#define DEFAULT_BUFLEN 1024
#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT "9999"
#define REQUEST_SIZE 6

static SOCKET getCommandListener(int* iRes);
static void wait_for_bot_request(SOCKET* listener, int* iRes, char* request);
static void get_constants(SOCKET* listener, int* iRes, CONSTANTS* constants);
static void set_exchanges(char** json, CONSTANTS* constants);
static void set_general_tickers(char** json, CONSTANTS* constants);
static void set_exchange_tickers(char** json, CONSTANTS* constants, string exchange);
static void set_exchange_tickers_sub_names(char** json, CONSTANTS* constants, string exchange);
static void set_exchange_tickers_margin(char** json, CONSTANTS* constants, string exchange);
static void set_exchange_tickers_price_steps(char** json, CONSTANTS* constants, string exchange);
static void set_general_symbols(char** json, CONSTANTS* constants);
static void set_exchange_symbols(char** json, CONSTANTS* constants, string exchange);
static void set_exchange_symbols_ids(char** json, CONSTANTS* constants, string exchange);
static void set_string_array_constant_field(char** json, const char* key, size_t* counter, string** field);
static void set_tickers_margin(char** json, size_t** margin);
static void set_tickers_price_step(char** json, double** price_step);
static void set_integer_constants_field(char** json, const char* key, size_t* field);
static void set_double_constants_field(char** json, const char* key, double* field);
static MAP* init_order_books(CONSTANTS* constants);
static void remove_order_books(MAP* order_books);
static void init_tickers_mapping(CONSTANTS* constants);
static void init_symbols_mapping(CONSTANTS* constants);
static void init_couples(CONSTANTS* constants);
static MAP* init_arbitrage_structs(CONSTANTS* constants);
static void init_api_keys(CONSTANTS* constants);
static void init_commissions(CONSTANTS* constants);
static MAP* init_balances(CONSTANTS* constants);
static void run_market_websockets(CONSTANTS* constants, MAP* ORDER_BOOKs, CONFIG* config);

#endif /* STRATEGY_H_ */
