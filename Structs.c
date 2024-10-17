/*
 * Structs.c
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */

#include "Structs.h"

static size_t str2number(char s);

extern LogLock;

char* zfill(int dig, int length)
{
	size_t len;
	char* z;
	char* str_dig;

	if (dig < 0){
		return NULL;
	}
	len = snprintf(NULL, 0, "%d", dig);
	if (len > length){
		return NULL;
	}

	z = (char*)malloc(length + 1);
	str_dig = (char*)malloc(len + 1);
	snprintf(str_dig, len + 1, "%d", dig);

	for (register int i = 0; i < length - len; i++){
		z[i] = '0';
	}
	strcpy(z + (length - len), str_dig);

	free(str_dig);

	return z;
}

char* orderbooks2json(MAP* orderbooks)
{
	char* json;
	char** tickers;

	json = (char*)malloc(2);
	strcpy(json, "{");
	tickers = map_get_entries(orderbooks);
	register int json_len = 1;

	for (register int i = 0; i < orderbooks->length; i++) {
		char* ticker;
		OrderBook* orderbook;
		MAP* asks;
		MAP* bids;

		ticker = tickers[i];
		json = (char*)realloc(json, json_len + strlen(ticker) + 6);
		strcat(json, "\"");
		strcat(json, ticker);
		strcat(json, "\": {");
		json_len += strlen(ticker) + 5;

		orderbook = (OrderBook*)map_get_value(orderbooks, ticker);

		asks = orderbook->asks;
		bids = orderbook->bids;

		json = (char*)realloc(json, json_len + 10);
		strcat(json, "\"asks\": {");
		json_len += 9;

		for (register size_t j = 0; j < asks->length; j++) {
			char* top;
			Tranch* tranch;
			double price;
			double size;
			char* json_tranch;
			size_t length;

			top = zfill(j, 3);
			tranch = (Tranch*)map_get_value(asks, top);

			price = tranch->price;
			size = tranch->size;

			length = snprintf(NULL, 0, "\"%s\": {\"price\": %lf, \"size\": %lf}", top, price, size);
			json_tranch = (char*)malloc(length + 1);
			snprintf(json_tranch, length+1, "\"%s\": {\"price\": %lf, \"size\": %lf}", top, price, size);

			json = (char*)realloc(json, json_len + length + 1);
			strcat(json, json_tranch);

			free(json_tranch);

			if (j != asks->length - 1) {
				json = (char*)realloc(json, json_len + length + 3);
				strcat(json, ", ");
				json_len += length + 2;
			}

			else {
				json = (char*)realloc(json, json_len + length + 4);
				strcat(json, "}, ");
				json_len += length + 3;
			}
		}

		json = (char*)realloc(json, json_len + 10);
		strcat(json, "\"bids\": {");
		json_len += 9;

		for (register size_t j = 0; j < bids->length; j++) {
			char* top;
			Tranch* tranch;
			double price;
			double size;
			char* json_tranch;
			size_t length;

			top = zfill(j, 3);
			tranch = (Tranch*)map_get_value(asks, top);

			price = tranch->price;
			size = tranch->size;

			length = snprintf(NULL, 0, "\"%s\": {\"price\": %lf, \"size\": %lf}", top, price, size);
			json_tranch = (char*)malloc(length + 1);
			snprintf(json_tranch, length+1, "\"%s\": {\"price\": %lf, \"size\": %lf}", top, price, size);

			json = (char*)realloc(json, json_len + length + 1);
			strcat(json, json_tranch);

			free(json_tranch);

			if (j != asks->length - 1) {
				json = (char*)realloc(json, json_len + length + 3);
				strcat(json, ", ");
				json_len += length + 2;
			}

			else {
				json = (char*)realloc(json, json_len + length + 3);
				strcat(json, "}}");
				json_len += length + 2;
			}
		}

		if (i != orderbooks->length - 1) {
			json = (char*)realloc(json, json_len + 3);
			strcat(json, ", ");
			json_len += 2;
		}

		else {
			json = (char*)realloc(json, json_len + 2);
			strcat(json, "}");
			json_len += 1;
		}
	}
	free(tickers);
	return json;
}

char* arbitragestruct2json(Arbitrage* arbitrage)
{
	char* json;
	char* ticker;
	char* taker;
	char* maker;
	double profit;
	char* profit_str;
	double usd_profit;
	char* usd_profit_str;
	double max_base_asset;
	char* max_base_asset_str;
	double max_quote_asset;
	char* max_quote_asset_str;
	double make_price;
	double make_size;
	char* make_price_str;
	char* make_size_str;
	size_t arbitrage_amount;
	Tranch** takes;
	char* quote_asset;
	char* base_asset;
	char* acc;
	char* sideMajor;

	json = (char*)malloc(13);
	strcpy(json, "{\"ticker\": \"");
	register int json_len = 12;

	//ticker
	ticker = arbitrage->ticker;
	json = (char*)realloc(json, json_len + strlen(ticker) + 4);
	strcat(json, ticker);
	strcat(json, "\", ");
	json_len += strlen(ticker) + 3;

	//taker exchange
	json = (char*)realloc(json, json_len + 20);
	strcat(json, "\"taker_exchange\": \"");
	json_len += 19;
	taker = arbitrage->taker;
	json = (char*)realloc(json, json_len + strlen(taker) + 4);
	strcat(json, taker);
	strcat(json, "\", ");
	json_len += strlen(taker) + 3;

	//maker exchange
	json = (char*)realloc(json, json_len + 20);
	strcat(json, "\"maker_exchange\": \"");
	json_len += 19;
	maker = arbitrage->maker;
	json = (char*)realloc(json, json_len + strlen(maker) + 4);
	strcat(json, maker);
	strcat(json, "\", ");
	json_len += strlen(maker) + 3;

	//max profit
	json = (char*)realloc(json, json_len + 15);
	strcat(json, "\"max_profit\": ");
	json_len += 14;
	profit = arbitrage->max_arbitrage_profit;
	register size_t len = snprintf(NULL, 0, "%.10f", profit);
	profit_str = (char*)malloc(len+1);
	snprintf(profit_str, len + 1, "%.10f", profit);
	json = (char*)realloc(json, json_len + strlen(profit_str) + 3);
	strcat(json, profit_str);
	strcat(json, ", ");
	json_len += strlen(profit_str) + 2;
	free(profit_str);

	//max profit in usd
	json = (char*)realloc(json, json_len + 22);
	strcat(json, "\"max_profit_in_usd\": ");
	json_len += 21;
	usd_profit = arbitrage->max_arbitrage_profit_in_usd;
	len = snprintf(NULL, 0, "%.10f", usd_profit);
	usd_profit_str = (char*)malloc(len + 1);
	snprintf(usd_profit_str, len + 1, "%.10f", usd_profit);
	json = (char*)realloc(json, json_len + strlen(usd_profit_str) + 3);
	strcat(json, usd_profit_str);
	strcat(json, ", ");
	json_len += strlen(usd_profit_str) + 2;
	free(usd_profit_str);

	//max base asset
	json = (char*)realloc(json, json_len + 19);
	strcat(json, "\"max_base_asset\": ");
	json_len += 18;
	max_base_asset = arbitrage->max_base_asset;
	len = snprintf(NULL, 0, "%.10f", max_base_asset);
	max_base_asset_str = (char*)malloc(len + 1);
	snprintf(max_base_asset_str, len + 1, "%.10f", max_base_asset);
	json = (char*)realloc(json, json_len + strlen(max_base_asset_str) + 3);
	strcat(json, max_base_asset_str);
	strcat(json, ", ");
	json_len += strlen(max_base_asset_str) + 2;
	free(max_base_asset_str);

	//max quote asset
	json = (char*)realloc(json, json_len + 20);
	strcat(json, "\"max_quote_asset\": ");
	json_len += 19;
	max_quote_asset = arbitrage->max_quote_asset;
	len = snprintf(NULL, 0, "%.10f", max_quote_asset);
	max_quote_asset_str = (char*)malloc(len + 1);
	snprintf(max_quote_asset_str, len + 1, "%.10f", max_quote_asset);
	json = (char*)realloc(json, json_len + strlen(max_quote_asset_str) + 3);
	strcat(json, max_quote_asset_str);
	strcat(json, ", ");
	json_len += strlen(max_quote_asset_str) + 2;
	free(max_quote_asset_str);

	//make
	json = (char*)realloc(json, json_len + 10);
	strcat(json, "\"make\": [");
	json_len += 9;
	make_price = arbitrage->make->price;
	make_size = arbitrage->make->size;
	len = snprintf(NULL, 0, "%.10f", make_price);
	make_price_str = (char*)malloc(len + 1);
	snprintf(make_price_str, len + 1, "%.10f", make_price);
	json = (char*)realloc(json, json_len + strlen(make_price_str) + 3);
	strcat(json, make_price_str);
	strcat(json, ", ");
	json_len += strlen(make_price_str) + 2;
	free(make_price_str);
	len = snprintf(NULL, 0, "%.10f", make_size);
	make_size_str = (char*)malloc(len + 1);
	snprintf(make_size_str, len + 1, "%.10f", make_size);
	json = (char*)realloc(json, json_len + strlen(make_size_str) + 4);
	strcat(json, make_size_str);
	strcat(json, "], ");
	json_len += strlen(make_size_str) + 3;
	free(make_size_str);

	//takes
	json = (char*)realloc(json, json_len + 11);
	strcat(json, "\"takes\": [");
	json_len += 10;
	arbitrage_amount = arbitrage->arbitrage_amount;
	takes = arbitrage->takes;
	for (register size_t i = 0; i < arbitrage_amount; i++) {
		Tranch* take;
		double take_price;
		double take_size;
		char* take_price_str;
		char* take_size_str;

		take = takes[i];
		take_price = take->price;
		take_size = take->size;
		len = snprintf(NULL, 0, "%.10f", take_price);
		take_price_str = (char*)malloc(len + 1);
		snprintf(take_price_str, len + 1, "%.10f", take_price);
		json = (char*)realloc(json, json_len + strlen(take_price_str) + 4);
		strcat(json, "[");
		strcat(json, take_price_str);
		strcat(json, ", ");
		json_len += strlen(take_price_str) + 3;
		free(take_price_str);
		len = snprintf(NULL, 0, "%.10f", take_size);
		take_size_str = (char*)malloc(len + 1);
		snprintf(take_size_str, len + 1, "%.10f", take_size);
		json = (char*)realloc(json, json_len + strlen(take_size_str) + 3);
		strcat(json, take_size_str);
		strcat(json, "] ");
		json_len += strlen(take_size_str) + 2;
		free(take_size_str);
		if (i < arbitrage_amount - 1) {
			json = (char*)realloc(json, json_len + 3);
			strcat(json, ", ");
			json_len += 2;
		}
	}
	json = (char*)realloc(json, json_len + 4);
	strcat(json, "], ");
	json_len += 3;

	//quote asset
	json = (char*)realloc(json, json_len + 17);
	strcat(json, "\"quote_asset\": \"");
	json_len += 16;
	quote_asset = arbitrage->quote_asset;
	json = (char*)realloc(json, json_len + strlen(quote_asset) + 4);
	strcat(json, quote_asset);
	strcat(json, "\", ");
	json_len += strlen(quote_asset) + 3;

	//base asset
	json = (char*)realloc(json, json_len + 16);
	strcat(json, "\"base_asset\": \"");
	json_len += 15;
	base_asset = arbitrage->base_asset;
	json = (char*)realloc(json, json_len + strlen(base_asset) + 4);
	strcat(json, base_asset);
	strcat(json, "\", ");
	json_len += strlen(base_asset) + 3;

	//account
	json = (char*)realloc(json, json_len + 13);
	strcat(json, "\"account\": \"");
	json_len += 12;
	acc = arbitrage->account;
	json = (char*)realloc(json, json_len + strlen(acc) + 4);
	strcat(json, acc);
	strcat(json, "\", ");
	json_len += strlen(acc) + 3;

	//side Major
	json = (char*)realloc(json, json_len + 15);
	strcat(json, "\"sideMajor\": \"");
	json_len += 14;
	sideMajor = arbitrage->sideMajor;
	json = (char*)realloc(json, json_len + strlen(sideMajor) + 3);
	strcat(json, sideMajor);
	strcat(json, "\"}");
	json_len += strlen(sideMajor) + 2;

	return json;
}

char* arbitrageinfo2json(char* data, char* type, char* taker, char* maker, char* ticker)
{
	unsigned long long ms_time;
	unsigned long long time;
	int ms;
	struct tm tm;
	char* ts;
	char* date;
	char* json;

	ms_time = GetTimeMs64();
	time = ms_time / (int)1000000;
	ms = ms_time - time * (int)1000000;
	tm = *localtime(&time);

	register size_t len = snprintf(NULL, 0, "%llu", ms_time);
	ts = (char*)malloc(len+1);
	snprintf(ts, len+1, "%llu", ms_time);
	len = snprintf(NULL, 0, "%d-%02d-%02d %02d:%02d:%02d.%06d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ms);
	date = (char*)malloc(len + 1);
	snprintf(date, len + 1, "%d-%02d-%02d %02d:%02d:%02d.%06d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ms);

	json = (char*)malloc(10);
	register int json_len = 9;
	strcpy(json, "\"type\": \"");

	json = (char*)realloc(json, json_len + strlen(type) + 4);
	strcat(json, type);
	strcat(json, "\", ");
	json_len += strlen(type) + 3;

	json = (char*)realloc(json, json_len + 10);
	strcat(json, "\"time\": \"");
	json_len += 9;
	json = (char*)realloc(json, json_len + strlen(ts) + 4);
	strcat(json, ts);
	strcat(json, "\", ");
	json_len += strlen(ts) + 3;

	json = (char*)realloc(json, json_len + 15);
	strcat(json, "\"date_time\": \"");
	json_len += 14;
	json = (char*)realloc(json, json_len + strlen(date) + 4);
	strcat(json, date);
	strcat(json, "\", ");
	json_len += strlen(date) + 3;

	if (!data) {
		data = "\"Arbitrage disappeared.\"";

		json = (char*)realloc(json, json_len + 11);
		strcat(json, "\"maker\": \"");
		json_len += 10;
		json = (char*)realloc(json, json_len + strlen(maker) + 4);
		strcat(json, maker);
		strcat(json, "\", ");
		json_len += strlen(maker) + 3;

		json = (char*)realloc(json, json_len + 11);
		strcat(json, "\"taker\": \"");
		json_len += 10;
		json = (char*)realloc(json, json_len + strlen(taker) + 4);
		strcat(json, taker);
		strcat(json, "\", ");
		json_len += strlen(taker) + 3;

		json = (char*)realloc(json, json_len + 12);
		strcat(json, "\"ticker\": \"");
		json_len += 11;
		json = (char*)realloc(json, json_len + strlen(ticker) + 4);
		strcat(json, ticker);
		strcat(json, "\", ");
		json_len += strlen(ticker) + 3;
	}

	json = (char*)realloc(json, json_len + 9);
	strcat(json, "\"data\": ");
	json_len += 8;
	json = (char*)realloc(json, json_len + strlen(data) + 2);
	strcat(json, data);
	strcat(json, "}");

	return json;
}

LOG_QUEUE* create_log_queue()
{
	LOG_QUEUE* log_queue;

	log_queue = (LOG_QUEUE*)malloc(sizeof(LOG_QUEUE));
	log_queue->length = 0;

	return log_queue;
}

void set_log(LOG_QUEUE* log_queue, char* log)
{
//	unsigned long dwWaitResult = WaitForSingleObject(LogLock, INFINITE);
	pthread_mutex_lock(&LogLock);
	if (log_queue->length == 0) {
		log_queue->queue = (struct LOG_NODE*)malloc(sizeof(struct LOG_NODE));
		log_queue->queue->val = log;
		log_queue->queue->next = NULL;
	}
	else {
		struct LOG_NODE* current = log_queue->queue;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = (struct LOG_NODE*)malloc(sizeof(struct LOG_NODE));
		current->next->val = log;
		current->next->next = NULL;
	}
	log_queue->length++;
	pthread_mutex_unlock(&LogLock);
//	ReleaseMutex(LogLock);
}

char* pop_log(LOG_QUEUE* log_queue)
{
	char* log;
	//unsigned long dwWaitResult = WaitForSingleObject(LogLock, INFINITE);
	pthread_mutex_lock(&LogLock);
	log = NULL;
	if (log_queue->length == 1) {
		log = log_queue->queue->val;
		free(log_queue->queue);
		log_queue->length--;
	}
	else if (log_queue->length > 1){
		log = log_queue->queue->val;
		struct LOG_NODE* tmp = log_queue->queue->next;
		free(log_queue->queue);
		log_queue->queue = tmp;
		log_queue->length--;
	}
	pthread_mutex_unlock(&LogLock);
	//ReleaseMutex(LogLock);

	return log;
}

void destroy_log_queue(LOG_QUEUE* log_queue)
{
	struct LOG_NODE* current;

	if (log_queue->length == 0) {
		free(log_queue);
		return;
	}
	current = log_queue->queue;
	while (log_queue->length) {
		struct LOG_NODE* tmp;

		tmp = current->next;
		free(current->val);
		free(current);
		current = tmp;
		log_queue->length--;
	}
	free(log_queue);
}

double str2double(char* elem)
{
	double ret = 0;
	size_t len = strlen(elem);
	if (!len){
		return 0;
	}
	char* neg_eps = strstr(elem, "E-");
	if (!neg_eps){
		neg_eps = strstr(elem, "e-");
	}
	char* pos_eps = strstr(elem, "E+");
	if (!pos_eps){
		pos_eps = strstr(elem, "e+");
	}
	char* dig;
	size_t e = NULL;
	if (neg_eps){
		size_t eps_index = neg_eps - elem;
		dig = (char*)malloc(eps_index + 1);
		strncpy(dig, elem, eps_index);
		e = str2number(elem[len - 1]);
	}

	else if (pos_eps){
		size_t eps_index = pos_eps - elem;
		dig = (char*)malloc(eps_index + 1);
		strncpy(dig, elem, eps_index);
		e = str2number(elem[len - 1]);
	}

	else{
		dig = (char*)malloc(len+1);
		strcpy(dig, elem);
	}

	char* dot = strstr(dig, ".");
	len = strlen(dig);
	if (dot){
		size_t dot_index = dot - dig;
		for (register int i = dot_index-1; i >= 0; i--){
			size_t n = str2number(dig[i]);

			if (n) ret += n * pow(10.0, (double)(dot_index - 1 - i));
		}

		for (register int i = dot_index+1; i < len; i++){
			size_t n = str2number(dig[i]);
			if (n) ret += n / pow(10.0, (double)(i - dot_index));
		}
	}

	else{
		for (register int i = len - 1; i >= 0; i--){
			size_t n = str2number(dig[i]);
			if (n) ret += n * pow(10.0, (double)(len - 1 - i));
		}
	}

	if ((e) && (neg_eps)){
		ret /= pow(10.0, (double)e);
	}
	if ((e) && (pos_eps)){
		ret *= pow(10.0, (double)e);
	}
	free(dig);

	return ret;
}

static size_t str2number(char s){
	if (s == '0') return 0;
	if (s == '1') return 1;
	if (s == '2') return 2;
	if (s == '3') return 3;
	if (s == '4') return 4;
	if (s == '5') return 5;
	if (s == '6') return 6;
	if (s == '7') return 7;
	if (s == '8') return 8;
	if (s == '9') return 9;
	else return NULL;
}
