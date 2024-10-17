/*
 * Binance.c
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */

#include "Binance.h"

static void generate_binance_signature(MAP* data, char* secret);
static size_t verify_price_ws_msg(string msg);

void initialize_binance_api_keys(CONSTANTS* constants)
{
	API_keys* api_keys;

	api_keys = (API_keys*)malloc(sizeof(API_keys));
	api_keys->public = "XllHxl5VFJ019zPZgqZJGEjOAi4ph50ud9aJJCVbpwz2lROqusvBWH9gwTP2cQqo";
	api_keys->secret = "sd896W7H9azIHhLqM61RhY7hvnuVHzfS55oWywIWxQJdPZeI9hWSbe8juaLVs9u6";
	map_set(constants->API_KEYS, "BINANCE", api_keys);
}

static void generate_binance_signature(MAP* data, char* secret)
{
	char* query_kv;
	uint8_t out[SHA256_HASH_SIZE];
	char* out_str;

	query_kv = format_query_key_val(data);
	out_str = (char*)malloc(SHA256_HASH_SIZE * 2 + 1);

	hmac_sha256(secret, strlen(secret), query_kv, strlen(query_kv), &out,
		sizeof(out));

	free(query_kv);

	// Convert `out` to string with printf
	for (register size_t i = 0; i < sizeof(out); i++) {
		snprintf(out_str + i*2, 3, "%02x", out[i]);
	}

	map_set(data, "signature", out_str);
}

THREAD initialize_binance_commissions(void* params)
{
	CONSTANTS* constants;
	MAP* tickers;
	MAP* data;
	unsigned long long ms;
	size_t len;
	char* ms_str;
	char* api_key;
	char* secret;
	MAP* headers;
	RESPONSE* response;
	char** entries;
	unsigned long begin;
	char* buffer;

	constants = (CONSTANTS*)params;
	tickers = (MAP*)map_get_value(constants->tickers_table, "BINANCE");

	for (register unsigned long i = 0; i < constants->general_tickers->length; i++) {
		string general_ticker;
		TickerProperty* property;

		general_ticker = ((string*)constants->general_tickers->entries)[i];
		property = (TickerProperty*)map_get_value(tickers, general_ticker);

		if (strcmp(property->name, "None") != 0) {
			Fees* fees;

			fees = (Fees*)malloc(sizeof(Fees));
			property->fees = fees;
		}
	}

	data = create_map();

	ms = GetTimeMs64() / (int)1000;
	len = snprintf(NULL, 0, "%llu", ms);
	ms_str = (char*)malloc(len + 1);
	snprintf(ms_str, len + 1, "%llu", ms);
	map_set(data, "timestamp", ms_str);

	api_key = ((API_keys*)map_get_value(constants->API_KEYS, "BINANCE"))->public;
	secret = ((API_keys*)map_get_value(constants->API_KEYS, "BINANCE"))->secret;

	generate_binance_signature(data, secret);

	headers = create_map();
	map_set(headers, "Accept", "application/json");
	map_set(headers, "User-Agent",
		"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Safari/537.36");
	map_set(headers, "X-MBX-APIKEY", api_key);

	response = NULL;
	while (1){
		response = httpRequest("https://api.binance.com", "/sapi/v1/asset/tradeFee", "GET", headers, data, NULL);
		if (response){
			break;
		}
		else{
			free((char*)map_get_value(data, "signature"));
			free((char*)map_get_value(data, "timestamp"));
			map_delete_key(data, "signature");
			ms = GetTimeMs64() / (int)1000;
			len = snprintf(NULL, 0, "%llu", ms);
			ms_str = (char*)malloc(len + 1);
			snprintf(ms_str, len + 1, "%llu", ms);
			map_set(data, "timestamp", ms_str);
			generate_binance_signature(data, secret);
		}
	}
	free((char*)map_get_value(data, "signature"));
	free((char*)map_get_value(data, "timestamp"));
	destroy_map(data);
	destroy_map(headers);

	if (response) {

		entries = (string*)constants->general_tickers->entries;
		buffer = (char*)malloc(16);
		for (register unsigned long i = 0; i < constants->general_tickers->length; i++) {
			string general_ticker;
			TickerProperty* property;
			char* maker_comm;
			char* taker_comm;

			general_ticker = entries[i];
			property = (TickerProperty*)map_get_value(tickers, general_ticker);

			if (strcmp(property->name, "None") == 0) continue;

			begin = strstr(response->resp_buf, property->name) - response->resp_buf;

			while (response->resp_buf[begin] != ',') begin++;
			maker_comm = (char*)malloc(16);
			begin += 20;
			register size_t j = 0;

			while (response->resp_buf[begin] != '"') {
				maker_comm[j] = response->resp_buf[begin];
				j++;
				begin++;
			}
			maker_comm[j] = '\0';

			taker_comm = (char*)malloc(16);
			begin += 21;
			j = 0;

			while (response->resp_buf[begin] != '"') {
				taker_comm[j] = response->resp_buf[begin];
				j++;
				begin++;
			}
			taker_comm[j] = '\0';

			property->fees->maker_fee = atof(maker_comm) * 100.0;
			free(maker_comm);

			property->fees->taker_fee = atof(taker_comm) * 100.0;
			free(taker_comm);
		}

		free(buffer);
		free(response->resp_buf);
		free(response);
	}

	else {
		printf("Can not read commissions from BINANCE.\n");
	}

	return 0;
}

THREAD initialize_binance_balance(void* params)
{
	CONSTANTS* constants;
	MAP* balance_state;

	constants = ((BALANCE_REQUEST_PARAMS*)params)->constants;
	balance_state = ((BALANCE_REQUEST_PARAMS*)params)->balance_state;
	free(params);
	initialize_binance_spot_balance(constants, balance_state);
	initialize_binance_margin_balance(constants, balance_state);

	return 0;
}

static void initialize_binance_spot_balance(CONSTANTS* constants, MAP* balance_state)
{
	MAP* data;
	unsigned long long ms;
	size_t len;
	char* ms_str;
	char* api_key;
	char* secret;
	MAP* headers;
	RESPONSE* response;

	data = create_map();

	ms = GetTimeMs64() / (int)1000;
	len = snprintf(NULL, 0, "%llu", ms);
	ms_str = (char*)malloc(len + 1);
	snprintf(ms_str, len + 1, "%llu", ms);
	map_set(data, "timestamp", ms_str);

	api_key = ((API_keys*)map_get_value(constants->API_KEYS, "BINANCE"))->public;
	secret = ((API_keys*)map_get_value(constants->API_KEYS, "BINANCE"))->secret;

	generate_binance_signature(data, secret);

	headers = create_map();
	map_set(headers, "Accept", "application/json");
	map_set(headers, "User-Agent",
		"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Safari/537.36");
	map_set(headers, "X-MBX-APIKEY", api_key);

	response = NULL;
	while (1){
		response = httpRequest("https://api.binance.com", "/api/v3/account", "GET", headers, data, NULL);
		if (response){
			break;
		}
		else{
			free((char*)map_get_value(data, "signature"));
			free((char*)map_get_value(data, "timestamp"));
			map_delete_key(data, "signature");
			ms = GetTimeMs64() / (int)1000;
			len = snprintf(NULL, 0, "%llu", ms);
			ms_str = (char*)malloc(len + 1);
			snprintf(ms_str, len + 1, "%llu", ms);
			map_set(data, "timestamp", ms_str);
			generate_binance_signature(data, secret);
		}
	}

	free((char*)map_get_value(data, "signature"));
	free((char*)map_get_value(data, "timestamp"));
	destroy_map(data);
	destroy_map(headers);

	if (response) {
		MAP* symbols;

		symbols = (MAP*)map_get_value(constants->symbols_table, "BINANCE");

		for (register unsigned long i = 0; i < constants->general_symbols->length; i++) {
			string general_symbol;
			SymbolProperty* property;

			general_symbol = ((string*)constants->general_symbols->entries)[i];
			property = (SymbolProperty*)map_get_value(symbols, general_symbol);

			if (strcmp(property->name, "None") != 0) {
				unsigned long begin;
				char* str_balance;
				Balance* balance;

				begin = strstr(response->resp_buf, property->name) - response->resp_buf;
				begin = strstr(response->resp_buf + begin, "free") - response->resp_buf + 7;
				str_balance = (char*)malloc(32);
				register size_t n = 0;
				while (response->resp_buf[begin] != '\"'){
					str_balance[n] = response->resp_buf[begin];
					n++;
					begin++;
				}

				str_balance[n] = '\0';
				balance = (Balance*)map_get_value(balance_state, general_symbol);
				balance->spot->free = atof(str_balance);
				free(str_balance);
			}
		}

		free(response->resp_buf);
		free(response);
	}
}

static void initialize_binance_margin_balance(CONSTANTS* constants, MAP* balance_state)
{
	MAP* data;
	unsigned long long ms;
	size_t len;
	char* ms_str;
	char* api_key;
	char* secret;
	MAP* headers;
	RESPONSE* response;

	data = create_map();

	ms = GetTimeMs64() / (int)1000;
	len = snprintf(NULL, 0, "%llu", ms);
	ms_str = (char*)malloc(len + 1);
	snprintf(ms_str, len + 1, "%llu", ms);
	map_set(data, "timestamp", ms_str);

	api_key = ((API_keys*)map_get_value(constants->API_KEYS, "BINANCE"))->public;
	secret = ((API_keys*)map_get_value(constants->API_KEYS, "BINANCE"))->secret;

	generate_binance_signature(data, secret);

	headers = create_map();
	map_set(headers, "Accept", "application/json");
	map_set(headers, "User-Agent",
		"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Safari/537.36");
	map_set(headers, "X-MBX-APIKEY", api_key);

	response = NULL;
	while (1){
		response = httpRequest("https://api.binance.com", "/sapi/v1/margin/account", "GET", headers, data, NULL);
		if (response){
			break;
		}
		else{
			free((char*)map_get_value(data, "signature"));
			free((char*)map_get_value(data, "timestamp"));
			map_delete_key(data, "signature");
			ms = GetTimeMs64() / (int)1000;
			len = snprintf(NULL, 0, "%llu", ms);
			ms_str = (char*)malloc(len + 1);
			snprintf(ms_str, len + 1, "%llu", ms);
			map_set(data, "timestamp", ms_str);
			generate_binance_signature(data, secret);
		}
	}

	free((char*)map_get_value(data, "signature"));
	free((char*)map_get_value(data, "timestamp"));
	destroy_map(data);
	destroy_map(headers);

	if (response) {
		MAP* symbols;

		symbols = (MAP*)map_get_value(constants->symbols_table, "BINANCE");

		for (register unsigned long i = 0; i < constants->general_symbols->length; i++) {
			string general_symbol;
			SymbolProperty* property;

			general_symbol = ((string*)constants->general_symbols->entries)[i];
			property = (SymbolProperty*)map_get_value(symbols, general_symbol);

			if (strcmp(property->name, "None") != 0) {
				Balance* balance;

				if (strstr(response->resp_buf, property->name)) {
					unsigned long begin;
					char* str_balance;

					begin = strstr(response->resp_buf, property->name) - response->resp_buf;
					begin = strstr(response->resp_buf + begin, "free") - response->resp_buf + 7;
					str_balance = (char*)malloc(32);
					register size_t n = 0;
					while (response->resp_buf[begin] != '\"'){
						str_balance[n] = response->resp_buf[begin];
						n++;
						begin++;
					}

					str_balance[n] = '\0';
					balance = (Balance*)map_get_value(balance_state, general_symbol);
					balance->cross_margin->free = atof(str_balance);
					free(str_balance);
				}
			}
		}

		free(response->resp_buf);
		free(response);
	}
}

void run_binance_market_websockets(CONSTANTS* constants, MAP* order_books, CONFIG* conf)
{
	const char* Uri = "wss://stream.binance.com/ws";
	MAP* Binance_Tickers;

	Binance_Tickers = (MAP*)map_get_value(constants->tickers_table, "BINANCE");

	char** entries = map_get_entries(Binance_Tickers);

	for (register unsigned long i = 0; i < constants->general_tickers->length; i++)
	{
		string general_ticker;
		TickerProperty* property;
		string ticker;
		string stream;
		OrderBook* order_book;

		general_ticker = ((string*)constants->general_tickers->entries)[i];
		property = (TickerProperty*)map_get_value(Binance_Tickers, general_ticker);

		ticker = property->name;
		stream = property->sub_name;
		order_book = (OrderBook*)map_get_value(order_books, general_ticker);

		if (strcmp(ticker, "None") != 0)
		{
			WS_PARAMS_BINANCE* params;
			pthread_t tid;

			params = (WS_PARAMS_BINANCE*)malloc(sizeof(WS_PARAMS_BINANCE));
			params->number = i;
			params->uri = Uri;
			params->general_ticker = general_ticker;
			params->ticker = ticker;
			params->stream = stream;
			params->order_book = order_book;
			params->run = &(conf->run);
			CREATE_THREAD(binance_order_book_stream, params, tid);
		}
	}

	free(entries);
}

static void read_order_book(char** json, const char* key, OrderBook* order_book)
{
	uint16_t begin;
	size_t elem_number;
	size_t j;
	MAP* current;
	char* price;
	char* size;

	if (strcmp(key, "asks") == 0){
		current = order_book->asks;
	}
	else{
		current = order_book->bids;
	}

	begin = strstr(*json, key) - *json;
	elem_number = 0;
	goto read_json;

read_elem:
	price = malloc(32);
	size = malloc(32);
	j = 0;
	while ((*json)[begin] != ',')
	{
		if ((*json)[begin] == '\"') begin++;
		else {
			price[j] = (*json)[begin];
			begin++;
			j++;
		}
	}
	price[j] = '\0';
	if (j > 32) {
		printf("PRICE: %s\n", *json);
	}
	price = (char*)realloc(price, j + 1);

	begin++;
	j = 0;
	while ((*json)[begin] != ']')
	{
		if ((*json)[begin] == '\"') begin++;
		else {
			size[j] = (*json)[begin];
			begin++;
			j++;
		}
	}
	size[j] = '\0';
	if (j > 32) {
		printf("SIZE: %s\n", *json);
	}
	size = (char*)realloc(size, j + 1);

	char* top = zfill(elem_number, 3);
	elem_number++;

	Tranch* tranch = (Tranch*)map_get_value(current, top);

	tranch->price = atof(price);
	tranch->size = atof(size);

	free(price);
	free(size);

	begin++;
	free(top);
	goto read_json;

read_json:
	while ((*json)[begin] != ']')
	{
		if ((*json)[begin] == '[' || (*json)[begin] == ',')
		{
			(begin) += 2;
			goto read_elem;
		}

		else
		{
			(begin)++;
		}
	}
}

static void handle_order_book_event(TLS_SOCKET* sock, const char** general_ticker, OrderBook* order_book, int* iRes)
{
	RESPONSE* response;
	char* msg;

	response = RECV(sock);
	if (response == NULL) {
		printf("Binance %s ws disconnected.\n", *general_ticker);
		*iRes = -1;
		return;
	}
	msg = response->resp_buf;
//	printf("MSG: %s\n", msg);
	if (verify_price_ws_msg(msg)) {

		read_order_book(&msg, "bids", order_book);
		read_order_book(&msg, "asks", order_book);
		order_book->live = 1;
	}
	free(msg);
	free(response);

	*iRes = 0;
}

static size_t verify_price_ws_msg(string msg)
{
	char* a = strstr(msg, "asks");
	char* b = strstr(msg, "bids");

	size_t c = msg[0] == '{';
	size_t d = msg[strlen(msg) - 1] == '}';

	return a && b && c && d;
}

static TLS_SOCKET* create_order_book_ws(const char** Uri, const char** general_ticker, const char** stream, size_t* number)
{
	TLS_SOCKET* sock;
	char* command;
	uint8_t len;
	char* str_num;

	sock = create_websocket_client(Uri);

	if (sock != NULL) {
		printf("Binance %s ws started.\n", *general_ticker);
	}

	else {
		return NULL;
	}

	len = snprintf(NULL, 0, "%zu", *number);
	str_num = (char*)malloc(len + 1);
	snprintf(str_num, len + 1, "%zu", *number);

	command = malloc(strlen(*stream) + strlen(str_num) + 128);
	strcpy(command, "{\"method\" : \"SUBSCRIBE\", \"params\" : [\"");
	strcat(command, *stream);
	strcat(command, "\"], \"id\" : ");
	strcat(command, str_num);
	strcat(command, "}");

	SEND(sock, &command);

	free(str_num);
	free(command);

	return sock;
}

static THREAD binance_order_book_stream(void* params)
{
	size_t number = ((WS_PARAMS_BINANCE*)params)->number;
	const char* Uri = ((WS_PARAMS_BINANCE*)params)->uri;
	const char* general_ticker = ((WS_PARAMS_BINANCE*)params)->general_ticker;
	const char* stream = ((WS_PARAMS_BINANCE*)params)->stream;
	size_t* run = ((WS_PARAMS_BINANCE*)params)->run;

	OrderBook* order_book = ((WS_PARAMS_BINANCE*)params)->order_book;
	free(params);

	TLS_SOCKET* sock = NULL;

	while (*run) {
		sock = create_order_book_ws(&Uri, &general_ticker, &stream, &number);
		if (sock == NULL) {
			//printf("Can not connect to %s ws Binance.\n", general_ticker);
		}
		else break;
	}

	if (sock == NULL) return 0;

	int iRes = 0;
	size_t reconnection_attempt = 0;
	while (*run) {
		handle_order_book_event(sock, &general_ticker, order_book, &iRes);

		while (iRes < 0){
			order_book->live = 0;
			if (reconnection_attempt >= 10) {
				sleep(60);
				reconnection_attempt = 0;
			}
			printf("Reconnecting %s websocket Binance...\n", general_ticker);
			if (sock) close_websocket_client(sock);
			sock = create_order_book_ws(&Uri, &general_ticker, &stream, &number);
			reconnection_attempt++;
			if (sock == NULL) {
				printf("Can not reconnect %s websocket Binance! Next trial...\n", general_ticker);
			}
			else iRes = 0;
		}
	}

	order_book->live = 0;
	close_websocket_client(sock);

	return 0;
}
