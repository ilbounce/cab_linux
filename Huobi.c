/*
 * Huobi.c
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */

#include "Huobi.h"

static void read_order_book(char* json, const char* key, OrderBook* order_book);

void initialize_huobi_api_keys(CONSTANTS* constants)
{
	API_keys* api_keys;

	api_keys = (API_keys*)malloc(sizeof(API_keys));
	api_keys->public = "2e18b620-bg2hyw2dfg-4194570a-b5838";
	api_keys->secret = "403786eb-db453ef8-adb21026-6886f";
	map_set(constants->API_KEYS, "HUOBI", api_keys);
}

static void generate_huobi_signature(MAP* data, char* secret, const char* Uri, char* path)
{
	char* qs0;
	char* payload;
	uint8_t out[SHA256_HASH_SIZE];
	char* b64;

	qs0 = format_query_key_val(data);
	register int l = snprintf(NULL, 0, "%s\n%s\n%s\n%s", "GET", Uri + 8, path, qs0);
	payload = (char*)malloc(l + 1);
	snprintf(payload, l + 1, "%s\n%s\n%s\n%s", "GET", Uri + 8, path, qs0);

	hmac_sha256(secret, strlen(secret), payload, strlen(payload), &out,
		sizeof(out));

	free(qs0);
	free(payload);

	b64 = base64_encode(out, SHA256_HASH_SIZE);
	while ((strstr(b64, "+")) || (strstr(b64, "=")) || (strstr(b64, "/"))) {
		if (strstr(b64, "+")) {
			size_t pos;
			char* end;

			pos = strstr(b64, "+") - b64;
			end = (char*)malloc(strlen(b64 + pos));
			strcpy(end, b64 + pos + 1);
			b64 = (char*)realloc(b64, strlen(b64) + 3);
			b64[pos] = '\0';
			strcat(b64, "%2B");
			strcat(b64, end);
			free(end);
		}
		if (strstr(b64, "=")) {
			size_t pos;
			char* end;

			pos = strstr(b64, "=") - b64;
			end = (char*)malloc(strlen(b64 + pos));
			strcpy(end, b64 + pos + 1);
			b64 = (char*)realloc(b64, strlen(b64) + 3);
			b64[pos] = '\0';
			strcat(b64, "%3D");
			strcat(b64, end);
			free(end);
		}
		if (strstr(b64, "/")) {
			size_t pos;
			char* end;

			pos = strstr(b64, "/") - b64;
			end = (char*)malloc(strlen(b64 + pos));
			strcpy(end, b64 + pos + 1);
			b64 = (char*)realloc(b64, strlen(b64) + 3);
			b64[pos] = '\0';
			strcat(b64, "%2F");
			strcat(b64, end);
			free(end);
		}
	}
	map_set(data, "Signature", b64);
}

THREAD initialize_huobi_commissions_pack(void* params)
{
	char* symbols;
	char** pack;
	size_t len_pack;
	MAP* headers;
	MAP* tickers_table;
	char* api_key;
	char* secret;
	MAP* data;
	char* URI;
	char* path;
	time_t utc_now;
	struct tm tm;
	char enc1[4] = "%3A";
	char* timestamp;
	RESPONSE* response;

	symbols = ((COMMISIONS_REQUEST_PARAMS_HUOBI*)params)->symbols;
	pack = ((COMMISIONS_REQUEST_PARAMS_HUOBI*)params)->pack;
	len_pack = ((COMMISIONS_REQUEST_PARAMS_HUOBI*)params)->len_pack;
	headers = create_map();
	map_set(headers, "Content-Type", "application/json");
	tickers_table = ((COMMISIONS_REQUEST_PARAMS_HUOBI*)params)->tickers_table;
	api_key = ((COMMISIONS_REQUEST_PARAMS_HUOBI*)params)->api_key;
	secret = ((COMMISIONS_REQUEST_PARAMS_HUOBI*)params)->secret;
	free((COMMISIONS_REQUEST_PARAMS_HUOBI*)params);

	data = create_map();

	URI = "https://api.huobi.pro";
	path = "/v2/reference/transact-fee-rate";

	map_set(data, "AccessKeyId", api_key);
	map_set(data, "SignatureVersion", "2");
	map_set(data, "SignatureMethod", "HmacSHA256");

	register size_t len = strlen(symbols);
	symbols = (char*)realloc(symbols, len - 2);
	symbols[len - 3] = '\0';

	utc_now = time(NULL);
	tm = *gmtime(&utc_now);
	register size_t l = snprintf(NULL, 0, "%d-%02d-%02dT%02d%s%02d%s%02d",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, enc1, tm.tm_min, enc1, tm.tm_sec);
	timestamp = (char*)malloc(l + 1);
	snprintf(timestamp, l + 1, "%d-%02d-%02dT%02d%s%02d%s%02d",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, enc1, tm.tm_min, enc1, tm.tm_sec);

	map_set(data, "Timestamp", timestamp);
	map_set(data, "symbols", symbols);

	generate_huobi_signature(data, secret, URI, path);

	response = NULL;
	while (1) {
		response = httpRequest(URI, path, "GET", headers, data, NULL);
		if (response) {
			break;
		}
		else {
			free((char*)map_get_value(data, "Signature"));
			free((char*)map_get_value(data, "Timestamp"));
			map_delete_key(data, "Signature");
			utc_now = time(NULL);
			tm = *gmtime(&utc_now);
			l = snprintf(NULL, 0, "%d-%02d-%02dT%02d%s%02d%s%02d",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, enc1, tm.tm_min, enc1, tm.tm_sec);
			timestamp = (char*)malloc(l + 1);
			snprintf(timestamp, l + 1, "%d-%02d-%02dT%02d%s%02d%s%02d",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, enc1, tm.tm_min, enc1, tm.tm_sec);

			map_set(data, "Timestamp", timestamp);
			generate_huobi_signature(data, secret, URI, path);
		}
	}

	free((char*)map_get_value(data, "Signature"));
	free((char*)map_get_value(data, "Timestamp"));

	if (response) {
		unsigned long begin;

		begin = 0;

		for (register int j = 0; j < len_pack; j++) {
			char* general_ticker;
			TickerProperty* property;
			char* maker_comm;
			char* taker_comm;

			general_ticker = pack[j];
			property = (TickerProperty*)map_get_value(tickers_table, general_ticker);
			begin = strstr(response->resp_buf, property->name) - response->resp_buf;

			maker_comm = (char*)malloc(16);
			begin = strstr(response->resp_buf + begin, "makerFeeRate") - response->resp_buf + 15;

			register size_t k = 0;
			while (response->resp_buf[begin + k] != '"') {
				maker_comm[k] = response->resp_buf[begin + k];
				maker_comm[k + 1] = '\0';
				k++;
			}

			taker_comm = (char*)malloc(16);
			begin = strstr(response->resp_buf + begin, "takerFeeRate") - response->resp_buf + 15;
			k = 0;

			while (response->resp_buf[begin + k] != '"') {
				taker_comm[k] = response->resp_buf[begin + k];
				taker_comm[k + 1] = '\0';
				k++;
			}

			property->fees->maker_fee = atof(maker_comm) * 100.0;
			free(maker_comm);

			property->fees->taker_fee = atof(taker_comm) * 100.0;
			free(taker_comm);
		}

		free(response->resp_buf);
		free(response);
	}

	else {
		printf("Can not read commissions from HUOBI.\n");
	}

	free(symbols);
	free(pack);
	destroy_map(data);
	destroy_map(headers);

	return 0;
}
THREAD initialize_huobi_commissions(void* params)
{
	CONSTANTS* constants;
	MAP* tickers;
	char* api_key;
	char* secret;
	char** entries;
	size_t thread_num;
	pthread_t tid[32];
	string* symbols_arr;
	string** packs;
	size_t len_pack;
	size_t max_len_pack;

	constants = (CONSTANTS*)params;
	tickers = (MAP*)map_get_value(constants->tickers_table, "HUOBI");

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

	api_key = ((API_keys*)map_get_value(constants->API_KEYS, "HUOBI"))->public;
	secret = ((API_keys*)map_get_value(constants->API_KEYS, "HUOBI"))->secret;

	entries = (string*)constants->general_tickers->entries;
	thread_num = 0;

	len_pack = 0;
	max_len_pack = 9;
	symbols_arr = (string*)malloc(32 * sizeof(string));
	symbols_arr[thread_num] = (string)malloc(128);
	strcpy(symbols_arr[thread_num], "");
	packs = (string**)malloc(32 * sizeof(string*));
	packs[thread_num] = (string*)malloc(max_len_pack * sizeof(string));

	for (register unsigned long i = 0; i < constants->general_tickers->length; i++) {
		string general_ticker;
		TickerProperty* property;

		if (len_pack == max_len_pack) {

			string* pack;
			string symbols;
			COMMISIONS_REQUEST_PARAMS_HUOBI* params;

			pack = packs[thread_num];
			symbols = symbols_arr[thread_num];
			params = (COMMISIONS_REQUEST_PARAMS_HUOBI*)malloc(sizeof(COMMISIONS_REQUEST_PARAMS_HUOBI));
			params->api_key = api_key;
			params->secret = secret;
			params->pack = pack;
			params->len_pack = len_pack;
			params->symbols = symbols;
			params->tickers_table = tickers;

			CREATE_THREAD(initialize_huobi_commissions_pack, (void*)params, tid[thread_num]);

			thread_num++;
			symbols_arr[thread_num] = (string)malloc(128);
			strcpy(symbols_arr[thread_num], "");
			packs[thread_num] = (string*)malloc(max_len_pack * sizeof(string));
			len_pack = 0;
		}

		general_ticker = entries[i];
		property = (TickerProperty*)map_get_value(tickers, general_ticker);
		if (strcmp(property->name, "None") == 0) continue;
		strcat(symbols_arr[thread_num], property->name);
		strcat(symbols_arr[thread_num], "%2C");
		packs[thread_num][len_pack] = general_ticker;
		len_pack++;
	}
	if (len_pack) {
		string* pack;
		string symbols;
		COMMISIONS_REQUEST_PARAMS_HUOBI* params;

		pack = packs[thread_num];
		symbols = symbols_arr[thread_num];
		params = (COMMISIONS_REQUEST_PARAMS_HUOBI*)malloc(sizeof(COMMISIONS_REQUEST_PARAMS_HUOBI));
		params->api_key = api_key;
		params->secret = secret;
		params->pack = pack;
		params->len_pack = len_pack;
		params->symbols = symbols;
		params->tickers_table = tickers;

		CREATE_THREAD(initialize_huobi_commissions_pack, (void*)params, tid[thread_num]);
	}

	for (register size_t i = 0; i <= thread_num; i++){
		pthread_join(tid[i], NULL);
	}

	free(symbols_arr);
	free(packs);

	return 0;
}

THREAD initialize_huobi_balance(void* params)
{
	CONSTANTS* constants;
	MAP* balance_state;
	MAP* data;
	const char* URI = "https://api.huobi.pro";
	char* path = "/v1/account/accounts/33045865/balance";
	char* api_key;
	char* secret;
	MAP* headers;
	time_t utc_now;
	struct tm tm;
	char enc1[4] = "%3A";
	size_t l;
	char* timestamp;
	RESPONSE* response;

	constants = ((BALANCE_REQUEST_PARAMS*)params)->constants;
	balance_state = ((BALANCE_REQUEST_PARAMS*)params)->balance_state;
	free(params);

	data = create_map();

	api_key = ((API_keys*)map_get_value(constants->API_KEYS, "HUOBI"))->public;
	secret = ((API_keys*)map_get_value(constants->API_KEYS, "HUOBI"))->secret;

	headers = create_map();
	map_set(headers, "Content-Type", "application/json");

	map_set(data, "AccessKeyId", api_key);
	map_set(data, "SignatureVersion", "2");
	map_set(data, "SignatureMethod", "HmacSHA256");

	utc_now = time(NULL);
	tm = *gmtime(&utc_now);
	l = snprintf(NULL, 0, "%d-%02d-%02dT%02d%s%02d%s%02d",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, enc1, tm.tm_min, enc1, tm.tm_sec);
	timestamp = (char*)malloc(l + 1);
	snprintf(timestamp, l + 1, "%d-%02d-%02dT%02d%s%02d%s%02d",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, enc1, tm.tm_min, enc1, tm.tm_sec);

	map_set(data, "Timestamp", timestamp);

	generate_huobi_signature(data, secret, URI, path);

	response = NULL;
	while (1) {
		response = httpRequest(URI, path, "GET", headers, data, NULL);
		if (response) {
			break;
		}
		else {
			free((char*)map_get_value(data, "Signature"));
			free((char*)map_get_value(data, "Timestamp"));
			map_delete_key(data, "Signature");
			utc_now = time(NULL);
			tm = *gmtime(&utc_now);
			l = snprintf(NULL, 0, "%d-%02d-%02dT%02d%s%02d%s%02d",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, enc1, tm.tm_min, enc1, tm.tm_sec);
			timestamp = (char*)malloc(l + 1);
			snprintf(timestamp, l + 1, "%d-%02d-%02dT%02d%s%02d%s%02d",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, enc1, tm.tm_min, enc1, tm.tm_sec);

			map_set(data, "Timestamp", timestamp);
			generate_huobi_signature(data, secret, URI, path);
		}
	}

	free((char*)map_get_value(data, "Signature"));
	free((char*)map_get_value(data, "Timestamp"));

	destroy_map(data);
	destroy_map(headers);

	if (response) {
		MAP* symbols;

		symbols = (MAP*)map_get_value(constants->symbols_table, "HUOBI");

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
					begin = strstr(response->resp_buf + begin, "trade") - response->resp_buf;
					begin = strstr(response->resp_buf + begin, "balance") - response->resp_buf + 10;

					str_balance = (char*)malloc(16);

					register size_t j = 0;

					while (response->resp_buf[begin + j] != '\"') {
						str_balance[j] = response->resp_buf[begin + j];
						j++;
					}

					str_balance[j] = '\0';
					balance = (Balance*)map_get_value(balance_state, general_symbol);

					balance->cross_margin->free = atof(str_balance);
					free(str_balance);
				}
			}
		}

		free(response->resp_buf);
		free(response);
	}

	return 0;
}


void run_huobi_market_websockets(CONSTANTS* constants, MAP* order_books, CONFIG* conf)
{
	const char* Uri = "wss://api.huobi.pro/ws";
	MAP* Huobi_Tickers;

	Huobi_Tickers = (MAP*)map_get_value(constants->tickers_table, "HUOBI");

	for (register unsigned long i = 0; i < constants->general_tickers->length; i++)
	{
		string general_ticker;
		TickerProperty* property;
		string ticker;
		OrderBook* order_book;

		general_ticker = ((string*)constants->general_tickers->entries)[i];
		property = (TickerProperty*)map_get_value(Huobi_Tickers, general_ticker);
		ticker = property->name;
		order_book = (OrderBook*)map_get_value(order_books, general_ticker);
		if (strcmp(ticker, "None") != 0)
		{
			WS_PARAMS_HUOBI* params;
			pthread_t tid;

			params = malloc(sizeof(WS_PARAMS_HUOBI));
			params->number = i;
			params->uri = Uri;
			params->general_ticker = general_ticker;
			params->ticker = ticker;
			params->order_book = order_book;
			params->run = &(conf->run);
			CREATE_THREAD(huobi_order_book_stream, params, tid);
		}
	}
}

static void read_order_book(char* json, const char* key, OrderBook* order_book)
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
	begin = strstr(json, key) - json;
	elem_number = 0;
	goto read_json;

read_elem:
	price = (char*)malloc(128);
	size = (char*)malloc(128);
	j = 0;
	while (json[begin] != ',')
	{
		price[j] = json[begin];
		begin++;
		j++;
	}
	price[j] = '\0';
	price = (char*)realloc(price, j + 1);
	begin++;
	j = 0;
	while (json[begin] != ']')
	{
		size[j] = json[begin];
		begin++;
		j++;
	}
	size[j] = '\0';
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
	while (json[begin] != ']')
	{
		if (json[begin] == '[' || json[begin] == ',')
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
	RESPONSE* response = RECV(sock);
	if (response == NULL) {
		printf("Huobi %s ws disconnected.\n", *general_ticker);
		*iRes = -1;
		return;
	}
	char* gzip_msg = response->resp_buf;
	uint32_t msg_length = response->resp_length;
	char msg[2048];

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	strm.avail_in = msg_length;
	strm.next_in = (Bytef*)gzip_msg;
	strm.avail_out = (uInt)sizeof(msg);
	strm.next_out = (Bytef*)msg;

	inflateInit2(&strm, MAX_WBITS | 16);
	inflate(&strm, Z_NO_FLUSH);
	inflateEnd(&strm);
	msg[strm.total_out] = '\0';

	if (strstr(msg, "status") != NULL) {}
	else if (strstr(msg, "tick") != NULL)
	{
		read_order_book(msg, "bids", order_book);
		read_order_book(msg, "asks", order_book);
		order_book->live = 1;
	}
	else if (strstr(msg, "ping"))
	{
		char pong[14];
		register size_t begin = strstr(msg, ":") - msg + 1;
		register size_t i = 0;
		do
		{
			pong[i] = msg[begin];
			begin++;
			i++;
		} while (msg[begin] != '}');
		pong[i] = '\0';
		char* command = malloc(27);
		sprintf(command, "{ \"pong\" : %s }", pong);
		SEND(sock, &command);
		free(command);
	}

	free(gzip_msg);
	free(response);

	*iRes = 0;
}

static TLS_SOCKET* create_order_book_ws(const char** Uri, const char** general_ticker, const char** ticker, size_t* number)
{
	TLS_SOCKET* sock;
	char* command;
	uint8_t len;
	char* str_num;

	sock = create_websocket_client(Uri);

	if (sock != NULL) {
		printf("Huobi %s ws started.\n", *general_ticker);
	}

	else {
		return NULL;
	}

	len = snprintf(NULL, 0, "%zu", *number);
	str_num = (char*)malloc(len+1);
	snprintf(str_num, len+1, "%zu", *number);

	command = malloc(strlen(*ticker) + strlen(str_num) + 64);
	strcpy(command, "{\"sub\" : \"market.");
	strcat(command, *ticker);
	strcat(command, ".mbp.refresh.10\", \"id\" : \"");
	strcat(command, str_num);
	strcat(command, "\"}");

	SEND(sock, &command);
	free(str_num);
	free(command);

	return sock;
}

static THREAD huobi_order_book_stream(void* params)
{
	size_t number = ((WS_PARAMS_HUOBI*)params)->number;
	const char* Uri = ((WS_PARAMS_HUOBI*)params)->uri;
	const char* general_ticker = ((WS_PARAMS_HUOBI*)params)->general_ticker;
	const char* ticker = ((WS_PARAMS_HUOBI*)params)->ticker;
	size_t* run = ((WS_PARAMS_HUOBI*)params)->run;

	OrderBook* order_book = ((WS_PARAMS_HUOBI*)params)->order_book;
	free(params);

	TLS_SOCKET* sock = NULL;

	for (register size_t i = 0; i < 5; i++) {
		sock = create_order_book_ws(&Uri, &general_ticker, &ticker, &number);
		if (sock != NULL) break;
	}

	if (sock == NULL) {
		printf("Can not connect to %s ws Huobi.\n", general_ticker);
		return 0;
	}

	int iRes = 0;
	size_t reconnection_attempt = 0;
	while (*run) {
		handle_order_book_event(sock, &general_ticker, order_book, &iRes);
		while (iRes < 0) {
			order_book->live = 0;
			if (reconnection_attempt >= 10) {
				sleep(60);
				reconnection_attempt = 0;
			}
			printf("Reconnecting %s websocket Huobi...\n", general_ticker);
			if (sock) close_websocket_client(sock);
			sock = create_order_book_ws(&Uri, &general_ticker, &ticker, &number);
			reconnection_attempt++;
			if (sock == NULL) {
				printf("Can not reconnect %s websocket Huobi! Next trial...\n", general_ticker);
			}
			else iRes = 0;
		}
	}

	order_book->live = 0;
	close_websocket_client(sock);

	return 0;
}
