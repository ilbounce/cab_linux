/*
 * Detector.h
 *
 *  Created on: May 25, 2023
 *      Author: ilbounce
 */

#ifndef DETECTOR_H_
#define DETECTOR_H_

#include "Structs.h"
#include "Logger.h"

typedef struct {
	Couple* couple;
	string general_ticker;
	double epsilon;
	size_t topN;
	OrderBook* order_book_left;
	OrderBook* order_book_right;
	TickerProperty* property_left;
	TickerProperty* property_right;
	ArbitrageTable* arbitrage_table;
	OrderBook* order_book_usdt;

	size_t* run;
	LOG_QUEUE* log_queue;
} DETECTOR_PARAMS;

typedef struct {
	Couple* couple;
	string general_ticker;
	OrderBook* order_book_left;
	OrderBook* order_book_right;
	TickerProperty* property_left;
	TickerProperty* property_right;
	ArbitrageTable* arbitrage_table;
	OrderBook* order_book_usdt;
	size_t topN;
	double epsilon;
	double ask_maker;
	double ask_taker;
	double bid_maker;
	double bid_taker;
	double fee;
} DETECTOR_CONFIG;

void run_detector(CONSTANTS* constants, GENERAL_STATES* states, CONFIG* conf, LOG_QUEUE* log_queue);
static size_t ticker_available(CONSTANTS* constants, char* general);
static size_t check_loading_ended(OrderBook* order_book);
static void arbitrage_function(DETECTOR_CONFIG* config);
static void arbitrage_constructor(DETECTOR_CONFIG* config, size_t maker_number, string side_major);
static void arbitrage_destructor(DETECTOR_CONFIG* config);
static THREAD start_detection(void* params);

#endif /* DETECTOR_H_ */
