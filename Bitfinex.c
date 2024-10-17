/*
 * Bitfinex.c
 *
 *  Created on: May 25, 2023
 *      Author: ilbounce
 */


#include "Bitfinex.h"

MAP* SnapShots;
MUTEX SnapShotDelLock;

static TLS_SOCKET* create_order_book_ws(const char** Uri, const char** general_ticker, const char** ticker, const char** subId);

static void handle_order_book_event(TLS_SOCKET* sock, const char** general_ticker, OrderBook* order_book, int* iRes, char** chanId, size_t topN);

static void read_order_book(const char** general_ticker, OrderBook* order_book, size_t topN);

void initialize_bitfinex_api_keys(CONSTANTS* constants)
{
	API_keys* api_keys;

	api_keys = (API_keys*)malloc(sizeof(API_keys));
	api_keys->public = "hN06JoRP9HJsgnJk2fbDmpTBdHYWvfXVW0ARHKrLZhc";
	api_keys->secret = "0roAi5RKk36QUSNPiWJUwbhOZL7uwbgthxYWaOujsR6";
	map_set(constants->API_KEYS, "BITFINEX", api_keys);
}

static void generate_bitfinex_signature(MAP* data, char* path, char* nonce, char* json, char* secret)
{
	unsigned long len = snprintf(NULL, 0, "/api/v2/%s%s%s", path, nonce, json);
	char* query = (char*)malloc(len + 1);
	snprintf(query, len + 1, "/api/v2/%s%s%s", path, nonce, json);

	uint8_t out[SHA384_HASH_SIZE];
	char* out_str = (char*)malloc(SHA384_HASH_SIZE * 2 + 1);

	hmac_sha384(secret, strlen(secret), query, strlen(query), &out,
		sizeof(out));

	free(query);

	// Convert `out` to string with printf
	for (register size_t i = 0; i < sizeof(out); i++) {
		snprintf(out_str + i * 2, 3, "%02x", out[i]);
	}

	//printf("%s\n", out_str);

	map_set(data, "bfx-signature", out_str);
}

THREAD initialize_bitfinex_commissions(void* params)
{
	CONSTANTS* constants;
	MAP* tickers;

	constants = (CONSTANTS*)params;
	tickers = (MAP*)map_get_value(constants->tickers_table, "BITFINEX");

	for (register int i = 0; i < constants->general_tickers->length; i++) {
		char* general_ticker;
		TickerProperty* property;
		char* ticker;

		general_ticker = ((string*)constants->general_tickers->entries)[i];
		property = (TickerProperty*)map_get_value(tickers, general_ticker);

		if (strcmp(property->name, "None") != 0) {

			property->fees = (Fees*)malloc(sizeof(Fees));
			property->fees->maker_fee = 0.1;
			property->fees->taker_fee = 0.2;
		}
	}

	return 0;
}

THREAD initialize_bitfinex_balance(void* params)
{
	CONSTANTS* constants;
	MAP* balance_state;
	MAP* headers;
	unsigned long long ms;
	size_t len;
	char* ms_str;
	const char* URI = "https://api.bitfinex.com";
	const char* path = "auth/r/wallets";
	char* api_key;
	char* secret;
	char* json;
	RESPONSE* response;

	constants = ((BALANCE_REQUEST_PARAMS*)params)->constants;
	balance_state = ((BALANCE_REQUEST_PARAMS*)params)->balance_state;
	free(params);
	headers = create_map();
	map_set(headers, "Content-Type", "application/json");

	ms = GetTimeMs64();
	len = snprintf(NULL, 0, "%llu", ms);
	ms_str = (char*)malloc(len + 1);
	snprintf(ms_str, len + 1, "%llu", ms);

	map_set(headers, "bfx-nonce", ms_str);

	api_key = ((API_keys*)map_get_value(constants->API_KEYS, "BITFINEX"))->public;
	secret = ((API_keys*)map_get_value(constants->API_KEYS, "BITFINEX"))->secret;

	json = "";

	map_set(headers, "bfx-apikey", api_key);
	generate_bitfinex_signature(headers, path, ms_str, json, secret);

	response = NULL;
	while (1){
		response = httpRequest(URI, "/v2/auth/r/wallets", "POST", headers, NULL, NULL);
		if (response){
			break;
		}
		else{
			free((char*)map_get_value(headers, "bfx-nonce"));
			free((char*)map_get_value(headers, "bfx-signature"));
			map_delete_key(headers, "bfx-nonce");
			map_delete_key(headers, "bfx-signature");
			ms = GetTimeMs64();
			len = snprintf(NULL, 0, "%llu", ms);
			ms_str = (char*)malloc(len + 1);
			snprintf(ms_str, len + 1, "%llu", ms);
			map_set(headers, "bfx-nonce", ms_str);
			json = "";
			generate_bitfinex_signature(headers, path, ms_str, json, secret);
		}
	}

	free((char*)map_get_value(headers, "bfx-nonce"));
	free((char*)map_get_value(headers, "bfx-signature"));

	destroy_map(headers);

	if (response) {
		MAP* symbols;

		symbols = (MAP*)map_get_value(constants->symbols_table, "BITFINEX");

		for (register unsigned long i = 0; i < constants->general_symbols->length; i++) {
			string general_symbol;
			SymbolProperty* property;

			general_symbol = ((string*)constants->general_symbols->entries)[i];
			property = (SymbolProperty*)map_get_value(symbols, general_symbol);

			if (strcmp(property->name, "None") != 0) {
				Balance* balance;

				if (strstr(response->resp_buf, property->name)) {
					char* cur_acc;
					unsigned long begin;
					char* acc;
					char* str_balance;
					char* another;

					cur_acc = (char*)malloc(8);
					begin = strstr(response->resp_buf, property->name) - response->resp_buf;
					register size_t j = 0;

					while (response->resp_buf[begin - 4 - j] != '"') {
						cur_acc[j] = response->resp_buf[begin - 4 - j];
						j++;
					}
					cur_acc[j] = '\0';

					if (strcmp(cur_acc, "nigram") == 0) {
						acc = "margin";
					}
					else {
						acc = "spot";
					}

					free(cur_acc);

					for (register size_t N = 0; N < 3; N++) {
						begin = strstr(response->resp_buf + begin, ",") - response->resp_buf;
					}
					begin++;

					str_balance = (char*)malloc(16);
					j = 0;

					while (response->resp_buf[begin + j] != ',') {
						str_balance[j] = response->resp_buf[begin + j];
						j++;
					}

					str_balance[j] = '\0';

					balance = (Balance*)map_get_value(balance_state, general_symbol);
					if (strcmp(acc, "margin") == 0){
						balance->cross_margin->free = atof(str_balance);
					}
					else{
						balance->spot->free = atof(str_balance);
					}

					free(str_balance);

					another = strcmp(acc, "margin") == 0 ? "spot" : "margin";

					if (strstr(response->resp_buf + begin, property->name)) {
						char* str_balance;

						begin = strstr(response->resp_buf + begin, property->name) - response->resp_buf;
						for (register size_t N = 0; N < 3; N++) {
							begin = strstr(response->resp_buf + begin, ",") - response->resp_buf;
						}
						begin++;

						str_balance = (char*)malloc(16);
						j = 0;

						while (response->resp_buf[begin + j] != ',') {
							str_balance[j] = response->resp_buf[begin + j];
							j++;
						}

						str_balance[j] = '\0';

						balance = (Balance*)map_get_value(balance_state, general_symbol);
						if (strcmp(another, "margin")){
							balance->cross_margin->free = atof(str_balance);
						}
						else{
							balance->spot->free = atof(str_balance);
						}
						free(str_balance);
					}
				}
			}
		}
	}

	return 0;
}

void run_bitfinex_market_websockets(CONSTANTS* constants, MAP* order_books, CONFIG* conf)
{
	const char* Uri = "wss://api-pub.bitfinex.com/ws/2";
	MAP* Bitfinex_Tickers;

	Bitfinex_Tickers = (MAP*)map_get_value(constants->tickers_table, "BITFINEX");

	SnapShots = create_map();

#ifdef WIN32
	SnapShotDelLock = CreateMutex(NULL, 0, NULL);
#elif __linux__
    pthread_mutex_init(&SnapShotDelLock, NULL);
#endif

	for (register unsigned long i = 0; i < constants->general_tickers->length; i++)
//	int k = 0;
//	for (register size_t i = k; i < k+1; i++)
	{
		string general_ticker;
		TickerProperty* property;
		string ticker;
		string subId;
		OrderBook* order_book;

		general_ticker = ((string*)constants->general_tickers->entries)[i];
		property = (TickerProperty*)map_get_value(Bitfinex_Tickers, general_ticker);
		ticker = property->name;
		subId = property->sub_name;
		order_book = (OrderBook*)map_get_value(order_books, general_ticker);
		if (strcmp(ticker, "None") != 0)
		{
			WS_PARAMS_BFX* params;
			pthread_t tid;

			params = malloc(sizeof(WS_PARAMS_BFX));
			params->number = i;
			params->uri = Uri;
			params->general_ticker = general_ticker;
			params->ticker = ticker;
			params->subId = subId;
			params->order_book = order_book;
			params->topN = conf->topN;
			params->run = &(conf->run);
			CREATE_THREAD(bitfinex_order_book_stream, params, tid);
		}
	}
}

static void set_snapshot(const char** general_ticker, char** json)
{
	MAP* snapshot_ask = create_map();
	MAP* snapshot_bid = create_map();
	uint16_t begin = strstr(*json, ",") - *json;
	begin++;
	char* price;
	char* size;
	size_t j;
	goto read_json;

read_elem:
	price = (char*)malloc(128);
	size = (char*)malloc(128);

	j = 0;
	while ((*json)[begin] != ',')
	{
		price[j] = (*json)[begin];
		begin++;
		j++;
	}
	price[j] = '\0';
	price = (char*)realloc(price, j + 1);
	begin++;
	while ((*json)[begin] != ',') begin++;
	begin++;

	j = 0;
	while ((*json)[begin] != ']')
	{
		size[j] = (*json)[begin];
		begin++;
		j++;
	}
	size[j] = '\0';
	size = (char*)realloc(size, j + 1);
	if (size[0] == '-') map_set_double(snapshot_ask, price, size);
	else map_set_double(snapshot_bid, price, size);
	begin++;
	goto read_json;

read_json:
	while ((*json)[begin] != ']')
	{
		if (((*json)[begin] == '[') || ((*json)[begin] == ',')) {
			begin += 2;
			goto read_elem;
		}
	}
	SNAPSHOT* snapshot = (SNAPSHOT*)malloc(sizeof(SNAPSHOT));
	snapshot->asks = snapshot_ask;
	snapshot->bids = snapshot_bid;
	map_set(SnapShots, *general_ticker, snapshot);
}

static void update_snapshot(const char** general_ticker, char** json)
{
	MAP* snapshot_ask;
	MAP* snapshot_bid;

	snapshot_ask = ((SNAPSHOT*)map_get_value(SnapShots, *general_ticker))->asks;
	snapshot_bid = ((SNAPSHOT*)map_get_value(SnapShots, *general_ticker))->bids;

	uint16_t begin = strstr(*json, ",") - *json;
	begin += 2;
	size_t j = 0;
	char* price = (char*)malloc(128);
	char* size = (char*)malloc(128);
	char* amt = (char*)malloc(16);

	while ((*json)[begin] != ',')
	{
		price[j] = (*json)[begin];
		begin++;
		j++;
	}
	price[j] = '\0';
	price = (char*)realloc(price, j + 1);
	begin++;

	j = 0;
	while ((*json)[begin] != ',')
	{
		amt[j] = (*json)[begin];
		begin++;
		j++;
	}
	amt[j] = '\0';
	amt = (char*)realloc(amt, j + 1);
	begin++;

	j = 0;
	while ((*json)[begin] != ']')
	{
		size[j] = (*json)[begin];
		begin++;
		j++;
	}
	size[j] = '\0';
	size = (char*)realloc(size, j + 1);

	if (strcmp(amt, "0") > 0) {
		if (size[0] == '-') {
			if (map_check_key_double(snapshot_ask, price) > 0) {
				double new_size = atof((char*)map_get_value_double(snapshot_ask, price)) + atof(size);
				size_t len = snprintf(NULL, 0, "%f", new_size);
				char* result = (char*)malloc(len + 1);
				snprintf(result, len + 1, "%f", new_size);

				free((char*)map_get_value_double(snapshot_ask, price));
				map_set_double(snapshot_ask, price, result);
				free(price);
				free(size);
				free(amt);
			}

			else {
				map_set_double(snapshot_ask, price, size);
				free(amt);
			}
		}
		else {
			if (map_check_key_double(snapshot_bid, price) > 0) {
				double new_size = atof((char*)map_get_value_double(snapshot_bid, price)) + atof(size);
				size_t len = snprintf(NULL, 0, "%f", new_size);
				char* result = (char*)malloc(len + 1);
				snprintf(result, len + 1, "%f", new_size);

				free((char*)map_get_value_double(snapshot_bid, price));
				map_set_double(snapshot_bid, price, result);
				free(price);
				free(size);
				free(amt);
			}

			else {
				map_set_double(snapshot_bid, price, size);
				free(amt);
			}
		}
	}

	else if (strcmp(amt, "0") == 0) {
		if (size[0] == '-') {
			if (map_check_key_double(snapshot_ask, price) != 0) {
				free((char*)map_get_value_double(snapshot_ask, price));
				char* key = map_delete_key_double(snapshot_ask, price);
				if (strcmp(key, "\0") != 0) free(key);
			}
			free(price);
			free(amt);
			free(size);
		}

		else {
			if (map_check_key_double(snapshot_bid, price) != 0) {
				free((char*)map_get_value_double(snapshot_bid, price));
				char* key = map_delete_key_double(snapshot_bid, price);
				if (strcmp(key, "\0") != 0) free(key);
			}
			free(price);
			free(amt);
			free(size);
		}
	}

	else {
		free(price);
		free(amt);
		free(size);
	}
}

static void read_order_book(const char** general_ticker, OrderBook* order_book, size_t topN)
{
	MAP* snapshot_ask;
	MAP* snapshot_bid;
	size_t elem_number;
	char** entries_ask;
	char** entries_bid;

	snapshot_ask = ((SNAPSHOT*)map_get_value(SnapShots, *general_ticker))->asks;
	snapshot_bid = ((SNAPSHOT*)map_get_value(SnapShots, *general_ticker))->bids;
	elem_number = 0;

	entries_ask = map_get_entries(snapshot_ask);

	for (register size_t iter = 0; iter < snapshot_ask->length; iter++) {
		if (elem_number == topN) {
			break;
		}
		char* new_price = entries_ask[iter];
		char* new_size = (char*)map_get_value_double(snapshot_ask, new_price);
		char* top = zfill(elem_number, 3);
		elem_number++;

		Tranch* tranch = (Tranch*)map_get_value(order_book->asks, top);
		tranch->price = atof(new_price);
		tranch->size = -atof(new_size);

		free(top);
	}
	free(entries_ask);

	elem_number = 0;
	entries_bid = map_get_entries(snapshot_bid);

	for (register int iter = snapshot_bid->length - 1; iter >= 0; iter--) {
		if (elem_number == topN) {
			break;
		}
		char* new_price = entries_bid[iter];
		char* new_size = (char*)map_get_value_double(snapshot_bid, new_price);
		char* top = zfill(elem_number, 3);
		elem_number++;

		Tranch* tranch = (Tranch*)map_get_value(order_book->bids, top);
		tranch->price = atof(new_price);
		tranch->size = atof(new_size);

		free(top);
	}

	free(entries_bid);
}

static void delete_snapshot(const char** general_ticker)
{
	if (map_check_key(SnapShots, *general_ticker) > 0) {
		MAP* snapshot_ask;
		MAP* snapshot_bid;

		snapshot_ask = ((SNAPSHOT*)map_get_value(SnapShots, *general_ticker))->asks;
		snapshot_bid = ((SNAPSHOT*)map_get_value(SnapShots, *general_ticker))->bids;

		char** entries_ask = map_get_entries(snapshot_ask);
		for (register size_t i = 0; i < snapshot_ask->length; i++) {
			free((char*)map_get_value_double(snapshot_ask, entries_ask[i]));
			free(entries_ask[i]);
		}
		destroy_map(snapshot_ask);
		free(entries_ask);

		char** entries_bid = map_get_entries(snapshot_bid);
		for (register size_t i = 0; i < snapshot_bid->length; i++) {
			free((char*)map_get_value_double(snapshot_bid, entries_bid[i]));
			free(entries_bid[i]);
		}
		destroy_map(snapshot_bid);
		free(entries_bid);

		free((SNAPSHOT*)map_get_value(SnapShots, *general_ticker));

		map_delete_key(SnapShots, *general_ticker);
	}
}

static void handle_order_book_event(TLS_SOCKET* sock, const char** general_ticker,
		OrderBook* order_book, int* iRes, char** chanId, size_t topN)
{

	RESPONSE* response = RECV(sock);
	if (response == NULL) {
		printf("Bintfinex %s ws disconnected.\n", *general_ticker);
#ifdef _WIN32
		DWORD dwWaitResult = WaitForSingleObject(SnapShotDelLock, INFINITE);
		delete_snapshot(general_ticker);
		ReleaseMutex(SnapShotDelLock);
#elif __linux__
		pthread_mutex_lock(&SnapShotDelLock);
		delete_snapshot(general_ticker);
		pthread_mutex_unlock(&SnapShotDelLock);
#endif
		*iRes = -1;
		return;
	}
	char* msg = response->resp_buf;

	if (response->resp_length < 2) {
		*iRes = 0;
		free(response);
		if (msg) free(msg);
		return;
	}

	if (strstr(msg, "chanId")) {
		*chanId = (char*)malloc(64);

		register unsigned int begin = strstr(msg, "chanId") - msg;
		while (msg[begin] != ':') begin++;
		begin++;
		register size_t i = 0;
		while (msg[begin] != ',') {
			(*chanId)[i] = msg[begin];
			begin++;
			i++;
		}
		(*chanId)[i] = '\0';

	}

	if (*chanId == NULL) {
		*iRes = 0;
		free(response);
		if (msg) free(msg);
		return;
	}

	if (strstr(msg, *chanId)) {
		if (strstr(msg, "{") == NULL)
		{
			if (strstr(msg, "hb") == NULL) {
				if (map_check_key(SnapShots, *general_ticker) == 0) {
					set_snapshot(general_ticker, &msg);
				}

				else
				{
					update_snapshot(general_ticker, &msg);
				}

				read_order_book(general_ticker, order_book, topN);
				order_book->live = 1;
			}
		}
	}

	free(response);
	free(msg);

	*iRes = 0;
}

static TLS_SOCKET* create_order_book_ws(const char** Uri, const char** general_ticker, const char** ticker, const char** subId)
{
	TLS_SOCKET* sock = create_websocket_client(Uri);

	if (sock != NULL) {
		printf("Bitfinex %s ws started.\n", *general_ticker);
	}

	else {
		return NULL;
	}

	char* command = (char*)malloc(strlen(*ticker) + strlen(*subId) + 128);
	strcpy(command, "{ \"event\": \"subscribe\", \"channel\": \"book\", \"prec\": \"P0\", \"symbol\": \"");
	strcat(command, *ticker);
	strcat(command, "\", \"subId\": \"");
	strcat(command, *subId);
	strcat(command, "\", \"len\": \"25\" }");

	SEND(sock, &command);

	free(command);

	return sock;
}

static THREAD bitfinex_order_book_stream(void* params)
{
	size_t number = ((WS_PARAMS_BFX*)params)->number;
	const char* Uri = ((WS_PARAMS_BFX*)params)->uri;
	const char* general_ticker = ((WS_PARAMS_BFX*)params)->general_ticker;
	const char* ticker = ((WS_PARAMS_BFX*)params)->ticker;
	const char* subId = ((WS_PARAMS_BFX*)params)->subId;
	size_t topN = ((WS_PARAMS_BFX*)params)->topN;
	size_t* run = ((WS_PARAMS_BFX*)params)->run;

	OrderBook* order_book = ((WS_PARAMS_BFX*)params)->order_book;
	free(params);

	TLS_SOCKET* sock = NULL;

	for (register size_t i = 0; i < 5; i++) {
		sock = create_order_book_ws(&Uri, &general_ticker, &ticker, &subId);
		if (sock != NULL) break;
	}

	if (sock == NULL) {
		printf("Can not connect to %s ws Bitfinex.\n", general_ticker);
		return 0;
	}

	int iRes = 0;
	char* chanId = NULL;
	size_t reconnection_attempt = 0;
	while (*run) {
		handle_order_book_event(sock, &general_ticker, order_book, &iRes, &chanId, topN);

		while (iRes < 0) {
			order_book->live = 0;
			if (sock) close_websocket_client(sock);
			free(chanId);
			chanId = NULL;
//			if (reconnection_attempt >= 10) {
//				printf("10 disconnections %s websocket Bitfinex. Sleep 60 seconds...\n", general_ticker);
//				sleep(60);
//				reconnection_attempt = 0;
//			}
			printf("%d Reconnecting %s websocket Bitfinex...\n", reconnection_attempt, general_ticker);
			sleep(1);
			sock = create_order_book_ws(&Uri, &general_ticker, &ticker, &subId);
			reconnection_attempt++;
			if (sock == NULL) {
				printf("Can not reconnect %s websocket Bitfinex!\n", general_ticker);
				//return 0;
			}
			else iRes = 0;
		}
	}

	order_book->live = 0;
	close_websocket_client(sock);
	delete_snapshot(&general_ticker);

	return 0;
}
