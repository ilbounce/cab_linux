/*
 * Bitfinex.h
 *
 *  Created on: May 25, 2023
 *      Author: ilbounce
 */

#ifndef BITFINEX_H_
#define BITFINEX_H_

#include "hmac.h"
#include "http.h"
#include "mstime.h"

typedef struct {
	size_t number;
	const char* uri;
	const char* general_ticker;
	const char* ticker;
	const char* subId;
	size_t topN;
	size_t* run;
	OrderBook* order_book;
} WS_PARAMS_BFX;

typedef struct {
	MAP* asks;
	MAP* bids;
} SNAPSHOT;

void initialize_bitfinex_api_keys(CONSTANTS* constants);
THREAD initialize_bitfinex_commissions(void* params);
static void generate_bitfinex_signature(MAP* data, char* path, char* nonce, char* json, char* secret);
THREAD initialize_bitfinex_balance(void* params);
void run_bitfinex_market_websockets(CONSTANTS* constants, MAP* order_books, CONFIG* conf);
static THREAD bitfinex_order_book_stream(void* params);
static void delete_snapshot(const char** general_ticker);
static void set_snapshot(const char** general_ticker, char** json);
static void update_snapshot(const char** general_ticker, char** json);

#endif /* BITFINEX_H_ */
