/*
 * Binance.h
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */

#ifndef BINANCE_H_
#define BINANCE_H_

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "http.h"
#include "hmac.h"
#include "mstime.h"

typedef struct {
	size_t number;
	const char* uri;
	const char* general_ticker;
	const char* ticker;
	const char* stream;
	size_t* run;
	OrderBook* order_book;
} WS_PARAMS_BINANCE;


void initialize_binance_api_keys(CONSTANTS* constants);
THREAD initialize_binance_commissions(void* params);
THREAD initialize_binance_balance(void* params);
static void initialize_binance_spot_balance(CONSTANTS* constants, MAP* balance_state);
static void initialize_binance_margin_balance(CONSTANTS* constants, MAP* balance_state);
void run_binance_market_websockets(CONSTANTS* constants, MAP* order_books, CONFIG* conf);
static THREAD binance_order_book_stream(void* params);
static TLS_SOCKET* create_order_book_ws(const char** Uri, const char** general_ticker, const char** stream, size_t* number);
static void handle_order_book_event(TLS_SOCKET* sock, const char** general_ticker, OrderBook* order_book, int* iRes);
static void read_order_book(char** json, const char* key, OrderBook* order_book);

#endif /* BINANCE_H_ */
