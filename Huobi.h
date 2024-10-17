/*
 * Huobi.h
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */

#ifndef HUOBI_H_
#define HUOBI_H_



#endif /* HUOBI_H_ */

#pragma once
#include <time.h>

#include "http.h"
#include "zlib.h"
#include "hmac.h"
#include "base64.h"

typedef struct {
	size_t number;
	const char* uri;
	const char* general_ticker;
	const char* ticker;
	size_t* run;
	OrderBook* order_book;
} WS_PARAMS_HUOBI;

typedef struct {
	char* symbols;
	char** pack;
	size_t len_pack;
	MAP* headers;
	MAP* tickers_table;
	char* secret;
	char* api_key;
} COMMISIONS_REQUEST_PARAMS_HUOBI;


void initialize_huobi_api_keys(CONSTANTS* constants);
THREAD initialize_huobi_commissions(void* params);
THREAD initialize_huobi_commissions_pack(void* params);
static void generate_huobi_signature(MAP* data, char* secret, const char* Uri, char* path);
THREAD initialize_huobi_balance(void* params);
void run_huobi_market_websockets(CONSTANTS* constants, MAP* order_books, CONFIG* conf);
static THREAD huobi_order_book_stream(void* params);
static TLS_SOCKET* create_order_book_ws(const char** Uri, const char** general_ticker, const char** ticker, size_t* number);
static void handle_order_book_event(TLS_SOCKET* sock, const char** general_ticker, OrderBook* order_book, int* iRes);
