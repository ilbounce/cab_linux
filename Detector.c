/*
 * Detector.c
 *
 *  Created on: May 25, 2023
 *      Author: ilbounce
 */

#include "Detector.h"

const string ASK = "ask";
const string BID = "bid";

void run_detector(CONSTANTS* constants, GENERAL_STATES* states, CONFIG* conf, LOG_QUEUE* log_queue)
{

	for (register unsigned long i = 0; i < constants->general_tickers->length; i++)
	{
		string general_ticker;

		general_ticker = ((string*)constants->general_tickers->entries)[i];
		if (ticker_available(constants, general_ticker)) {

			for (register unsigned long j = 0; j < constants->couples->length; j++) {

				Couple* couple;
				OrderBook* order_book_left;
				OrderBook* order_book_right;
				ArbitrageTable* arbitrage_table;
				MAP* tickers_left;
				MAP* tickers_right;
				TickerProperty* property_left;
				TickerProperty* property_right;
				pthread_t tid;

				couple = ((Couple**)constants->couples->entries)[j];

				order_book_left = (OrderBook*)map_get_value((MAP*)map_get_value(states->ORDER_BOOKs, couple->left), general_ticker);
				order_book_right = (OrderBook*)map_get_value((MAP*)map_get_value(states->ORDER_BOOKs, couple->right), general_ticker);

				arbitrage_table = (ArbitrageTable*)map_get_value((MAP*)map_get_value(states->ARBITRAGES, couple->value), general_ticker);

				tickers_left = (MAP*)map_get_value(constants->tickers_table, couple->left);
				tickers_right = (MAP*)map_get_value(constants->tickers_table, couple->right);

				property_left = (TickerProperty*)map_get_value(tickers_left, general_ticker);
				property_right = (TickerProperty*)map_get_value(tickers_right, general_ticker);

				if ((strcmp(property_left->name, "None") != 0) && (strcmp(property_right->name, "None") != 0)) {
					DETECTOR_PARAMS* params;

					params = (DETECTOR_PARAMS*)malloc(sizeof(DETECTOR_PARAMS));
					params->couple = couple;
					params->general_ticker = general_ticker;
					params->epsilon = constants->epsilon;
					params->topN = constants->topN;
					params->order_book_left = order_book_left;
					params->order_book_right = order_book_right;
					params->property_left = property_left;
					params->property_right = property_right;
					params->arbitrage_table = arbitrage_table;

					if ((strstr(general_ticker, "USDT") == NULL) || (strstr(general_ticker, "USDC") == NULL)) {
						if (strstr(general_ticker, "BTC")) {
							params->order_book_usdt = (OrderBook*)map_get_value((MAP*)map_get_value(states->ORDER_BOOKs, couple->left), "BTC+USDT");
						}
						else if (strstr(general_ticker, "ETH")) {
							params->order_book_usdt = (OrderBook*)map_get_value((MAP*)map_get_value(states->ORDER_BOOKs, couple->left), "ETH+USDT");
						}
						else params->order_book_usdt = NULL;
					}

					else {
						params->order_book_usdt = NULL;
					}

					params->run = &(conf->run);
					params->log_queue = log_queue;

					CREATE_THREAD(start_detection, params, tid);
				}
			}
		}
	}
}

static size_t ticker_available(CONSTANTS* constants, char* general)
{
	size_t N;

	N = 0;

	for (register size_t i = 0; i < constants->exchanges->length; i++) {
		string exchange;
		MAP* tickers;
		TickerProperty* property;

		exchange = ((string*)constants->exchanges->entries)[i];
		tickers = (MAP*)map_get_value(constants->tickers_table, exchange);

		property = (TickerProperty*)map_get_value(tickers, general);

		if (strcmp(property->name, "None") == 0) {
			N++;
		}
	}

	if (constants->exchanges->length - N > 1) return 1;
	else return 0;
}

static size_t check_loading_ended(OrderBook* order_book)
{
	return order_book->live;
}

static void arbitrage_constructor(DETECTOR_CONFIG* config, size_t maker_number, string side_major)
{
	string side_minor;
	size_t taker_number;
	size_t upd;
	ArbitrageTable* arbitrage_table;
	Arbitrage* arbitrage;
	double profit;
	Tranch* make;
	Tranch** takes;
	double* arbitrage_sizes;
	double best_size;
	size_t arbitrage_amount;
	double max_invest;
	char* top;

	side_minor = strcmp(side_major, BID) == 0 ? ASK : BID;
	taker_number = maker_number == 1 ? 0 : 1;

	upd = 0;

	if (maker_number == 0) {
		config->ask_maker -= config->property_left->price_step;
		config->bid_maker += config->property_left->price_step;
	}

	else {
		config->ask_maker -= config->property_right->price_step;
		config->bid_maker += config->property_right->price_step;
	}

	arbitrage_table = config->arbitrage_table;
	arbitrage = arbitrage_table->result;

	if (!arbitrage) {
		arbitrage = (Arbitrage*)malloc(sizeof(Arbitrage));
		arbitrage->ticker = config->general_ticker;
		arbitrage->taker = taker_number == 0 ? config->couple->left : config->couple->right;
		arbitrage->maker = maker_number == 0 ? config->couple->left : config->couple->right;
		arbitrage_table->result = arbitrage;
		arbitrage_table->new = 1;
	}

	else upd = 1;

	profit = 0;
	make = (Tranch*)malloc(sizeof(Tranch));
	takes = (Tranch**)malloc(1 * sizeof(Tranch*));

	arbitrage_sizes = (double*)malloc(1 * sizeof(double));
	top = zfill(0, 3);
	if (taker_number == 0) {
		if (strcmp(side_major, ASK) == 0) best_size = ((Tranch*)map_get_value(config->order_book_left->asks, top))->size;
		else best_size = ((Tranch*)map_get_value(config->order_book_left->bids, top))->size;
	}

	else {
		if (strcmp(side_major, ASK) == 0) best_size = ((Tranch*)map_get_value(config->order_book_right->asks, top))->size;
		else best_size = ((Tranch*)map_get_value(config->order_book_right->bids, top))->size;
	}

	free(top);

	make->price = strcmp(side_major, ASK) == 0 ? config->ask_maker : config->bid_maker;
	make->size = best_size;

	takes[0] = (Tranch*)malloc(sizeof(Tranch));
	takes[0]->price = strcmp(side_major, ASK) == 0 ? config->ask_taker : config->bid_taker;
	takes[0]->size = best_size;

	profit += (((1 - min(make->price, takes[0]->price) / max(make->price, takes[0]->price)) * 100.0 - config->fee) / 100.0) *
			(takes[0]->price * best_size);

	arbitrage_sizes[0] = (1 - min(make->price, takes[0]->price) / max(make->price, takes[0]->price)) * 100.0 - config->fee;

	arbitrage_amount = 1;

	for (register size_t i = 1; i < config->topN; i++) {
		double Nbest_price;

		top = zfill(i, 3);

		if (taker_number == 0) {
			if (strcmp(side_major, ASK) == 0) Nbest_price = ((Tranch*)map_get_value(config->order_book_left->asks, top))->price;
			else Nbest_price = ((Tranch*)map_get_value(config->order_book_left->bids, top))->price;
		}
		else {
			if (strcmp(side_major, ASK) == 0) Nbest_price = ((Tranch*)map_get_value(config->order_book_right->asks, top))->price;
			else Nbest_price = ((Tranch*)map_get_value(config->order_book_right->bids, top))->price;
		}

		free(top);

		if ((1 - min(make->price, Nbest_price) / max(make->price, Nbest_price)) * 100.0 - config->fee >= config->epsilon) {

			double Nbest_size;

			top = zfill(i, 3);

			if (taker_number == 0) {
				if (strcmp(side_major, ASK) == 0) Nbest_size = ((Tranch*)map_get_value(config->order_book_left->asks, top))->size;
				else Nbest_size = ((Tranch*)map_get_value(config->order_book_left->bids, top))->size;
			}
			else {
				if (strcmp(side_major, ASK) == 0) Nbest_size = ((Tranch*)map_get_value(config->order_book_right->asks, top))->size;
				else Nbest_size = ((Tranch*)map_get_value(config->order_book_right->bids, top))->size;
			}

			free(top);

			make->size += Nbest_size;
			takes = (Tranch**)realloc(takes, (i + 1) * sizeof(Tranch*));
			takes[i] = (Tranch*)malloc(sizeof(Tranch));
			takes[i]->price = Nbest_price;
			takes[i]->size = Nbest_size;

			profit += (((1 - min(make->price, takes[i]->price) / max(make->price, takes[i]->price)) * 100.0 - config->fee) / 100.0) *
				(takes[i]->price * Nbest_size);

			arbitrage_sizes = (double*)realloc(arbitrage_sizes, (i + 1) * sizeof(double));
			arbitrage_sizes[i] = (1 - min(make->price, takes[i]->price) / max(make->price, takes[i]->price)) * 100.0 - config->fee;
			arbitrage_amount++;
		}

		else break;
	}

	if (upd) {
		double prev_profit;
		size_t prev_arbitrage_amount;

		prev_profit = arbitrage->max_arbitrage_profit;

		if (prev_profit == profit) {
			arbitrage_table->new = 0;
			arbitrage_table->update = 1;
			return;
		}

		prev_arbitrage_amount = arbitrage->arbitrage_amount;
		free(arbitrage->make);
		free(arbitrage->arbitrage_sizes);
		for (register size_t i = 0; i < prev_arbitrage_amount; i++) {
			free(arbitrage->takes[i]);
		}
		free(arbitrage->takes);
		free(arbitrage->base_asset);
		arbitrage_table->new = 1;
		arbitrage_table->update = 1;
	}

	arbitrage->make = make;
	arbitrage->takes = takes;
	arbitrage->arbitrage_amount = arbitrage_amount;
	arbitrage->arbitrage_sizes = arbitrage_sizes;
	arbitrage->max_arbitrage_profit = profit;
	arbitrage->max_base_asset = make->size;

	max_invest = 0;
	for (register size_t i = 0; i < arbitrage_amount; i++) {
		max_invest += takes[i]->price * takes[i]->size;
	}

	arbitrage->max_quote_asset = max_invest;

	if (strstr(config->general_ticker, "USDT")) {
		string base_asset;

		arbitrage->quote_asset = "USDT";
		register size_t end = strstr(config->general_ticker, "USDT") - config->general_ticker - 1;
		base_asset = (string)malloc(end);
		register size_t i = 0;
		do {
			base_asset[i] = config->general_ticker[i];
			i++;
		} while (config->general_ticker[i] != '+');
		base_asset[i] = '\0';
		arbitrage->base_asset = base_asset;
		arbitrage->max_arbitrage_profit_in_usd = arbitrage->max_arbitrage_profit;
	}

	else if (strstr(config->general_ticker, "USDC")) {
		string base_asset;

		arbitrage->quote_asset = "USDC";
		register size_t end = strstr(config->general_ticker, "USDC") - config->general_ticker - 1;
		base_asset = (string)malloc(end);
		register size_t i = 0;
		do {
			base_asset[i] = config->general_ticker[i];
			i++;
		} while (config->general_ticker[i] != '+');
		base_asset[i] = '\0';

		arbitrage->base_asset = base_asset;
		arbitrage->max_arbitrage_profit_in_usd = arbitrage->max_arbitrage_profit;
	}

	else {
		double in_usd_price;
		double profit_in_usd;

		top = zfill(0, 3);

		in_usd_price = 0.5 * (((Tranch*)map_get_value(config->order_book_usdt->asks, top))->price +
				((Tranch*)map_get_value(config->order_book_usdt->bids, top))->price);

		free(top);

		if (strstr(config->general_ticker, "ETH")) {
			string base_asset;

			arbitrage->quote_asset = "ETH";
			register size_t end = strstr(config->general_ticker, "ETH") - config->general_ticker - 1;
			base_asset = (char*)malloc(end);
			register size_t i = 0;
			do {
				base_asset[i] = config->general_ticker[i];
				i++;
			} while (config->general_ticker[i] != '+');
			base_asset[i] = '\0';
			arbitrage->base_asset = base_asset;

		}
		else if (strstr(config->general_ticker, "BTC")) {
			string base_asset;

			arbitrage->quote_asset = "BTC";
			register size_t end = strstr(config->general_ticker, "BTC") - config->general_ticker - 1;
			base_asset = (char*)malloc(end);
			register size_t i = 0;
			do {
				base_asset[i] = config->general_ticker[i];
				i++;
			} while (config->general_ticker[i] != '+');
			base_asset[i] = '\0';
			arbitrage->base_asset = base_asset;
		}

		profit_in_usd = arbitrage->max_arbitrage_profit * in_usd_price;

		arbitrage->max_arbitrage_profit_in_usd = profit_in_usd;
	}

	if ((config->property_left->margin) && (config->property_right->margin)) arbitrage->account = "MARGIN";
	else arbitrage->account = "SPOT";

	arbitrage->sideMajor = side_major;

	arbitrage->taker_taker = 0;

	arbitrage_table->result = arbitrage;
	config->arbitrage_table = arbitrage_table;
}

static void arbitrage_destructor(DETECTOR_CONFIG* config)
{
	ArbitrageTable* arbitrage_table;
	Arbitrage* arbitrage;
	size_t prev_arbitrage_amount;

	arbitrage_table = config->arbitrage_table;
	arbitrage = arbitrage_table->result;

	arbitrage_table->taker = arbitrage->taker;
	arbitrage_table->maker = arbitrage->maker;

	prev_arbitrage_amount = arbitrage->arbitrage_amount;
	free(arbitrage->make);
	free(arbitrage->arbitrage_sizes);

	for (register size_t i = 0; i < prev_arbitrage_amount; i++) {
		free(arbitrage->takes[i]);
	}
	free(arbitrage->takes);
	free(arbitrage->base_asset);

	free(arbitrage);
	arbitrage_table->result = NULL;
	arbitrage_table->new = 0;
	arbitrage_table->update = 0;
	arbitrage_table->delete = 0;

	config->arbitrage_table = arbitrage_table;
}

static void arbitrage_function(DETECTOR_CONFIG* config)
{
	double best_ask_left;
	double best_ask_right;
	double best_bid_left;
	double best_bid_right;
	double fee_maker_taker;
	double fee_taker_maker;
	double max_val;
	double ask_taker_maker;
	double ask_maker_taker;
	double bid_taker_maker;
	double bid_maker_taker;

	char* top;

	top = zfill(0, 3);

	best_ask_left = ((Tranch*)map_get_value(config->order_book_left->asks, top))->price;
	best_ask_right = ((Tranch*)map_get_value(config->order_book_right->asks, top))->price;
	best_bid_left = ((Tranch*)map_get_value(config->order_book_left->bids, top))->price;
	best_bid_right = ((Tranch*)map_get_value(config->order_book_right->bids, top))->price;

	free(top);

	fee_maker_taker = config->property_left->fees->maker_fee + config->property_right->fees->taker_fee * 2 +
			config->property_left->fees->taker_fee;
	fee_taker_maker = config->property_left->fees->taker_fee * 2 + config->property_right->fees->maker_fee +
			config->property_right->fees->taker_fee;

	max_val = -100.0;

	ask_taker_maker = (1.0 - best_ask_left / (best_ask_right - config->property_right->price_step)) * 100.0 - fee_taker_maker;
	if (ask_taker_maker > max_val) max_val = ask_taker_maker;

	ask_maker_taker = (1.0 - best_ask_right / (best_ask_left - config->property_left->price_step)) * 100.0 - fee_maker_taker;
	if (ask_maker_taker > max_val) max_val = ask_maker_taker;

	bid_taker_maker = (best_bid_left / (best_bid_right + config->property_right->price_step) - 1.0) * 100.0 - fee_taker_maker;
	if (bid_taker_maker > max_val) max_val = bid_taker_maker;

	bid_maker_taker = (best_bid_right / (best_bid_left + config->property_left->price_step) - 1.0) * 100.0 - fee_maker_taker;
	if (bid_maker_taker > max_val) max_val = bid_maker_taker;

	if ((ask_taker_maker == max_val) && (ask_taker_maker >= config->epsilon)) {
		config->ask_maker = best_ask_right;
		config->bid_maker = best_bid_right;
		config->ask_taker = best_ask_left;
		config->bid_taker = best_bid_left;
		config->fee = fee_taker_maker;
		arbitrage_constructor(config, 1, ASK);
	}

	else if ((ask_maker_taker == max_val) && (ask_maker_taker >= config->epsilon)) {
		config->ask_maker = best_ask_left;
		config->bid_maker = best_bid_left;
		config->ask_taker = best_ask_right;
		config->bid_taker = best_bid_right;
		config->fee = fee_maker_taker;
		arbitrage_constructor(config, 0, ASK);
	}

	else if ((bid_taker_maker == max_val) && (bid_taker_maker >= config->epsilon)) {
		config->ask_maker = best_ask_right;
		config->bid_maker = best_bid_right;
		config->ask_taker = best_ask_left;
		config->bid_taker = best_bid_left;
		config->fee = fee_taker_maker;
		arbitrage_constructor(config, 1, BID);
	}

	else if ((bid_maker_taker == max_val) && (bid_maker_taker >= config->epsilon)) {
		config->ask_maker = best_ask_left;
		config->bid_maker = best_bid_left;
		config->ask_taker = best_ask_right;
		config->bid_taker = best_bid_right;
		config->fee = fee_maker_taker;
		arbitrage_constructor(config, 0, BID);
	}

	else if (config->arbitrage_table->result) {
		arbitrage_destructor(config);
	}
}

static THREAD start_detection(void* params)
{
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
	DETECTOR_CONFIG* config;

	couple = ((DETECTOR_PARAMS*)params)->couple;
	general_ticker = ((DETECTOR_PARAMS*)params)->general_ticker;
	epsilon = ((DETECTOR_PARAMS*)params)->epsilon;
	topN = ((DETECTOR_PARAMS*)params)->topN;
	order_book_left = ((DETECTOR_PARAMS*)params)->order_book_left;
	order_book_right = ((DETECTOR_PARAMS*)params)->order_book_right;
	property_left = ((DETECTOR_PARAMS*)params)->property_left;
	property_right = ((DETECTOR_PARAMS*)params)->property_right;
	arbitrage_table = ((DETECTOR_PARAMS*)params)->arbitrage_table;
	order_book_usdt = ((DETECTOR_PARAMS*)params)->order_book_usdt;
	run = ((DETECTOR_PARAMS*)params)->run;
	log_queue = ((DETECTOR_PARAMS*)params)->log_queue;

	config = (DETECTOR_CONFIG*)malloc(sizeof(DETECTOR_CONFIG));
	config->couple = couple;
	config->general_ticker = general_ticker;
	config->epsilon = epsilon;
	config->topN = topN;
	config->order_book_left = order_book_left;
	config->order_book_right = order_book_right;
	config->property_left = property_left;
	config->property_right = property_right;
	config->arbitrage_table = arbitrage_table;
	config->order_book_usdt = order_book_usdt;

	while (*run) {
		if ((check_loading_ended(order_book_left)) && (check_loading_ended(order_book_right))) {
			ArbitrageTable* arbitrage_table;
			Arbitrage* arbitrage;

			arbitrage_function(config);

			arbitrage_table = config->arbitrage_table;
			arbitrage = arbitrage_table->result;


			if ((!arbitrage_table->delete) && (!arbitrage)) { //no arbitrage
				continue;
			}

			else if ((arbitrage_table->update) && (!arbitrage_table->new)) { //no changes
				continue;
			}

			else if (arbitrage_table->delete) { //arbitrage disappeared
				string maker;
				string taker;

				printf("ARBITRAGE DISAPPEARED: %s, %s\n", config->couple->value, config->general_ticker);

				maker = arbitrage_table->maker;
				taker = arbitrage_table->taker;
				char* data = arbitrageinfo2json(NULL, "delete", taker, maker, config->general_ticker);
				arbitrage_table->delete = 0;
				arbitrage_table->maker = NULL;
				arbitrage_table->taker = NULL;
				log_msg(log_queue, data);
			}

			else if ((arbitrage_table->new) && (!arbitrage_table->update)) { //arbitrage appeared
				string json_result;
				string data;

				printf("ARBITRAGE APPEARED: %s, %s\n", config->couple->value, config->general_ticker);
				json_result = arbitragestruct2json(arbitrage);
				data = arbitrageinfo2json(json_result, "new", NULL, NULL, NULL);
				log_msg(log_queue, data);
			}

			else if ((arbitrage_table->new) && (arbitrage_table->update)) { //arbitrage updated
				string json_result;
				string data;

				printf("ARBITRAGE UPDATED: %s, %s\n", config->couple->value, config->general_ticker);
				json_result = arbitragestruct2json(arbitrage);
				data = arbitrageinfo2json(json_result, "change", NULL, NULL, NULL);
				log_msg(log_queue, data);
			}
		}
	}

	return 0;
}
