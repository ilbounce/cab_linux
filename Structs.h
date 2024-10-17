/*
 * Structs.h
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */

#ifndef STRUCTS_H_
#define STRUCTS_H_

#define min(X,Y) ((X) < (Y) ? (X) : (Y))
#define max(X,Y) ((X) > (Y) ? (X) : (Y))

#ifdef WIN32
#pragma once
#endif

#include "Map.h"
//#include <time.h>
#include "mstime.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define THREAD unsigned long __stdcall
#define CREATE_THREAD(func, params, id) CreateThread(NULL, 0, func, params, 0, &id);
#elif __linux__
#include <pthread.h>
#define THREAD void*
#define CREATE_THREAD(func, params, id) pthread_create(&id, NULL, func, params);
#endif

#ifdef WIN32
#define MUTEX void*
#elif __linux__
#define MUTEX pthread_mutex_t
#endif

#ifdef WIN32
//unsigned long dwWaitResult;
#endif

typedef char* string;

typedef struct {
	void* entries;
	unsigned long length;
} List;

typedef struct {
    char** NAMES;
    char** SUB_NAMES;
    size_t* MARGIN;
    double* PRICE_STEPS;
    MAP* commissions;
    size_t TICKERS_NUMBER;
    MAP* tickers2general;
    MAP* general2tickers;
    MAP* ticker2margin;
    MAP* ticker2pricestep;
} Tickers;

typedef struct {
	double maker_fee;
	double taker_fee;
} Fees;

typedef struct {
	char* name;
	char* sub_name;
	unsigned int margin;
	double price_step;
	Fees* fees;
	unsigned long number;
} TickerProperty;

typedef struct {
	char* name;
	char* id;
	unsigned long number;
} SymbolProperty;

typedef struct {
    char** NAMES;
    char** IDS;
    size_t SYMBOLS_NUMBER;
    MAP* symbols2general;
    MAP* general2symbols;
} Symbols;

typedef struct {
    char** NAMES;
    size_t TICKERS_NUMBER;
} GeneralTickers;

typedef struct {
    char** NAMES;
    size_t SYMBOLS_NUMBER;
} GeneralSymbols;

typedef struct {
	char* public;
	char* secret;
} API_keys;

typedef struct {
	string left;
	string right;
	string value;
} Couple;

typedef struct {
	List* exchanges;
	List* general_tickers;
	List* general_symbols;
	List* couples;
	MAP* tickers_table;
	MAP* symbols_table;
    MAP* API_KEYS;
    size_t topN;
	double epsilon;
} CONSTANTS;

typedef struct {
    size_t run;
    size_t topN;
} CONFIG;

typedef struct {
	double price;
	double size;
} Tranch;

typedef struct {
	string ticker;
	string taker;
	string maker;
	Tranch* make;
	Tranch** takes;
	string base_asset;
	string quote_asset;
	double* arbitrage_sizes;
	size_t arbitrage_amount;
	double max_arbitrage_profit;
	double max_arbitrage_profit_in_usd;
	double max_base_asset;
	double max_quote_asset;
	string account;
	string sideMajor;
	size_t taker_taker;
} Arbitrage;

typedef struct {
	Arbitrage* result;
	string maker;
	string taker;
	unsigned int new;
	unsigned int update;
	unsigned int delete;
} ArbitrageTable;

typedef struct {
    MAP* ORDER_BOOKs;
    MAP* BALANCEs;
    MAP* ARBITRAGES;
} GENERAL_STATES;

typedef struct {
	MAP* asks;
	MAP* bids;
	size_t live;
} OrderBook;

typedef struct {
	double free;
	double locked;
	double borrow;
	double maxBorrowable;
} Wallet;

typedef struct {
	Wallet* spot;
	Wallet* cross_margin;
} Balance;

typedef struct {
    CONSTANTS* constants;
    MAP* balance_state;
} BALANCE_REQUEST_PARAMS;

struct LOG_NODE {
    char* val;
    struct LOG_NODE* next;
};

typedef struct {
    struct LOG_NODE* queue;
    unsigned int length;
} LOG_QUEUE;

void init_mutex();

char* orderbooks2json(MAP* orderbooks);
char* arbitragestruct2json(Arbitrage* arbitrage);
char* arbitrageinfo2json(char* data, char* type, char* taker, char* maker, char* ticker);
LOG_QUEUE* create_log_queue();
void set_log(LOG_QUEUE* log_queue, char* log);
char* pop_log(LOG_QUEUE* log_queue);
void destroy_log_queue(LOG_QUEUE* log_queue);

char* zfill(int dig, int length);

double str2double(char* elem);

#endif /* STRUCTS_H_ */
