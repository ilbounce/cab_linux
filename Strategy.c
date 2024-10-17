/*
 * Strategy.c
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */
#include "Strategy.h"

MUTEX TLSLock;
MUTEX LogLock;

static SOCKET getCommandListener(int* iRes)
{
    const char* address = "127.0.0.1";
    const char* port = "9999";
    return create_server_connection(iRes, &address, &port);
}

static void wait_for_bot_request(SOCKET* listener, int* iRes, char* request)
{
    *iRes = recv(*listener, request, REQUEST_SIZE, 0);

    if (*iRes < 0)
    {
        printf("Receiving failed with error\n");
        close(*listener);
    }
}

static void set_exchanges(char** json, CONSTANTS* constants)
{
	char* key;
	uint16_t begin;
	unsigned int elem_number;
	unsigned int j;
	string* exchanges_lst;
	unsigned long new_size;
	char* new_json;

	key = "\"exchanges\"";
	begin = strstr(*json, key) - *json;
	elem_number = 0;
	constants->exchanges = (List*)malloc(sizeof(List));
	constants->exchanges->length = 0;
	exchanges_lst = (string*)malloc(sizeof(string));
	goto read_json;

read_elem:
	exchanges_lst = (string*)realloc(exchanges_lst, (1 + elem_number) * sizeof(string));
	j = 0;
	exchanges_lst[elem_number] = (string)malloc(1);
	while ((*json)[begin] != '\"')
	{
		exchanges_lst[elem_number] = (string)realloc(exchanges_lst[elem_number], (1 + j));
		exchanges_lst[elem_number][j] = (*json)[begin];
		(begin)++;
		j++;
	}

	exchanges_lst[elem_number] = (string)realloc(exchanges_lst[elem_number], (1 + j));
	exchanges_lst[elem_number][j] = '\0';
	constants->exchanges->length++;
	elem_number++;
	goto read_json;

read_json:
	while ((*json)[begin] != ']')
	{
		if ((*json)[begin] == '[')
		{
			(begin) += 2;
			goto read_elem;
		}

		else if ((*json)[begin] == ',')
		{
			(begin) += 3;
			goto read_elem;
		}

		else
		{
			(begin)++;
		}
	}

	begin += 3;
	constants->exchanges->entries = exchanges_lst;
	new_size = strlen(*json) - begin + 1;
	new_json = (char*)malloc(new_size);
	memcpy(new_json, (*json) + begin, new_size);
	free(*json);
	*json = new_json;
}

static void set_general_tickers(char** json, CONSTANTS* constants)
{
	char* key;
	uint16_t begin;
	unsigned int elem_number;
	unsigned int j;
	string* tickers_lst;
	unsigned long new_size;
	char* new_json;

	key = "\"TICKER\"";
	begin = strstr(*json, key) - *json;
	elem_number = 0;
	constants->general_tickers = (List*)malloc(sizeof(List));
	constants->general_tickers->length = 0;
	tickers_lst = (string*)malloc(sizeof(string));
	goto read_json;

read_elem:
	tickers_lst = (string*)realloc(tickers_lst, (1 + elem_number) * sizeof(string));
	j = 0;
	tickers_lst[elem_number] = (string)malloc(1);
	while ((*json)[begin] != '\"')
	{
		tickers_lst[elem_number] = (string)realloc(tickers_lst[elem_number], (1 + j));
		tickers_lst[elem_number][j] = (*json)[begin];
		(begin)++;
		j++;
	}

	tickers_lst[elem_number] = (string)realloc(tickers_lst[elem_number], (1 + j));
	tickers_lst[elem_number][j] = '\0';
	constants->general_tickers->length++;
	elem_number++;
	goto read_json;

read_json:
	while ((*json)[begin] != ']')
	{
		if ((*json)[begin] == '[')
		{
			(begin) += 2;
			goto read_elem;
		}

		else if ((*json)[begin] == ',')
		{
			(begin) += 3;
			goto read_elem;
		}

		else
		{
			(begin)++;
		}
	}

	begin += 3;
	constants->general_tickers->entries = tickers_lst;
	new_size = strlen(*json) - begin + 1;
	new_json = (char*)malloc(new_size);
	memcpy(new_json, (*json) + begin, new_size);
	free(*json);
	*json = new_json;
}

static void set_exchange_tickers(char** json, CONSTANTS* constants, string exchange)
{
	MAP* exchange_tickers_table;
	char* key;
	uint16_t begin;
	unsigned int elem_number;
	unsigned int j;
	string general_ticker;
	string exchange_ticker;
	TickerProperty* property;
	unsigned long new_size;
	char* new_json;

	exchange_tickers_table = create_map();
	key = "\"TICKER\"";
	begin = strstr(*json, key) - *json;
	elem_number = 0;
	goto read_json;

read_elem:
	general_ticker = ((string*)constants->general_tickers->entries)[elem_number];
	exchange_ticker = (string)malloc(1);
	property = (TickerProperty*)malloc(sizeof(TickerProperty));
	j = 0;
	while ((*json)[begin] != '\"')
	{
		exchange_ticker = (string)realloc(exchange_ticker, (1 + j));
		exchange_ticker[j] = (*json)[begin];
		(begin)++;
		j++;
	}

	exchange_ticker = (string)realloc(exchange_ticker, (1 + j));
	exchange_ticker[j] = '\0';

	property->name = exchange_ticker;
	property->number = elem_number;
	map_set(exchange_tickers_table, general_ticker, property);

	elem_number++;
	goto read_json;

read_json:
	while ((*json)[begin] != ']')
	{
		if ((*json)[begin] == '[')
		{
			(begin) += 2;
			goto read_elem;
		}

		else if ((*json)[begin] == ',')
		{
			(begin) += 3;
			goto read_elem;
		}

		else
		{
			(begin)++;
		}
	}

	begin += 3;
	map_set(constants->tickers_table, exchange, exchange_tickers_table);
	new_size = strlen(*json) - begin + 1;
	new_json = (char*)malloc(new_size);
	memcpy(new_json, (*json) + begin, new_size);
	free(*json);
	*json = new_json;
}

static void set_exchange_tickers_sub_names(char** json, CONSTANTS* constants, string exchange)
{
	MAP* exchange_tickers_table;
	char* key;
	uint16_t begin;
	unsigned int elem_number;
	unsigned int j;
	string general_ticker;
	string exchange_ticker_sub_name;
	TickerProperty* property;
	unsigned long new_size;
	char* new_json;

	exchange_tickers_table = (MAP*)map_get_value(constants->tickers_table, exchange);
	key = "\"SUB_IDS\"";
	begin = strstr(*json, key) - *json;
	elem_number = 0;
	goto read_json;

read_elem:
	general_ticker = ((string*)constants->general_tickers->entries)[elem_number];
	property = (TickerProperty*)map_get_value(exchange_tickers_table, general_ticker);
	exchange_ticker_sub_name = (string)malloc(1);
	j = 0;
	while ((*json)[begin] != '\"')
	{
		exchange_ticker_sub_name = (string)realloc(exchange_ticker_sub_name, (1 + j));
		exchange_ticker_sub_name[j] = (*json)[begin];
		(begin)++;
		j++;
	}

	exchange_ticker_sub_name = (string)realloc(exchange_ticker_sub_name, (1 + j));
	exchange_ticker_sub_name[j] = '\0';

	property->sub_name = exchange_ticker_sub_name;

	elem_number++;
	goto read_json;

read_json:
	while ((*json)[begin] != ']')
	{
		if ((*json)[begin] == '[')
		{
			(begin) += 2;
			goto read_elem;
		}

		else if ((*json)[begin] == ',')
		{
			(begin) += 3;
			goto read_elem;
		}

		else
		{
			(begin)++;
		}
	}

	begin += 3;
	new_size = strlen(*json) - begin + 1;
	new_json = (char*)malloc(new_size);
	memcpy(new_json, (*json) + begin, new_size);
	free(*json);
	*json = new_json;
}

static void set_exchange_tickers_margin(char** json, CONSTANTS* constants, string exchange)
{
	MAP* exchange_tickers_table;
	char* key;
	uint16_t begin;
	unsigned int elem_number;
	unsigned int j;
	string general_ticker;
	string exchange_ticker_margin;
	TickerProperty* property;
	unsigned long new_size;
	char* new_json;

	exchange_tickers_table = (MAP*)map_get_value(constants->tickers_table, exchange);
	key = "\"MARGIN\"";
	begin = strstr(*json, key) - *json;
	elem_number = 0;
	goto read_json;

read_elem:
	general_ticker = ((string*)constants->general_tickers->entries)[elem_number];
	property = (TickerProperty*)map_get_value(exchange_tickers_table, general_ticker);
	exchange_ticker_margin = (string)malloc(1);
	j = 0;
	while ((*json)[begin] != '\"')
	{
		exchange_ticker_margin = (string)realloc(exchange_ticker_margin, (1 + j));
		exchange_ticker_margin[j] = (*json)[begin];
		(begin)++;
		j++;
	}

	exchange_ticker_margin = (string)realloc(exchange_ticker_margin, (1 + j));
	exchange_ticker_margin[j] = '\0';

	property->margin = strcmp(exchange_ticker_margin, "True") == 0 ? 1 : 0;

	elem_number++;
	goto read_json;

read_json:
	while ((*json)[begin] != ']')
	{
		if ((*json)[begin] == '[')
		{
			(begin) += 2;
			goto read_elem;
		}

		else if ((*json)[begin] == ',')
		{
			(begin) += 3;
			goto read_elem;
		}

		else
		{
			(begin)++;
		}
	}

	begin += 3;
	new_size = strlen(*json) - begin + 1;
	new_json = (char*)malloc(new_size);
	memcpy(new_json, (*json) + begin, new_size);
	free(*json);
	*json = new_json;
}

static void set_exchange_tickers_price_steps(char** json, CONSTANTS* constants, string exchange)
{
	MAP* exchange_tickers_table;
	char* key;
	uint16_t begin;
	unsigned int elem_number;
	unsigned int j;
	string general_ticker;
	string exchange_ticker_price_step;
	TickerProperty* property;
	unsigned long new_size;
	char* new_json;

	exchange_tickers_table = (MAP*)map_get_value(constants->tickers_table, exchange);
	key = "\"STEP\"";
	begin = strstr(*json, key) - *json;
	elem_number = 0;
	goto read_json;

read_elem:
	general_ticker = ((string*)constants->general_tickers->entries)[elem_number];
	property = (TickerProperty*)map_get_value(exchange_tickers_table, general_ticker);
	exchange_ticker_price_step = (string)malloc(1);
	j = 0;
	while ((*json)[begin] != '\"')
	{
		exchange_ticker_price_step = (string)realloc(exchange_ticker_price_step, (1 + j));
		exchange_ticker_price_step[j] = (*json)[begin];
		(begin)++;
		j++;
	}

	exchange_ticker_price_step = (string)realloc(exchange_ticker_price_step, (1 + j));
	exchange_ticker_price_step[j] = '\0';

	property->price_step = strcmp(exchange_ticker_price_step, "None") == 0 ? 0 : atof(exchange_ticker_price_step);

	elem_number++;
	goto read_json;

read_json:
	while ((*json)[begin] != ']')
	{
		if ((*json)[begin] == '[')
		{
			(begin) += 2;
			goto read_elem;
		}

		else if ((*json)[begin] == ',')
		{
			(begin) += 3;
			goto read_elem;
		}

		else
		{
			(begin)++;
		}
	}

	begin += 3;
	new_size = strlen(*json) - begin + 1;
	new_json = (char*)malloc(new_size);
	memcpy(new_json, (*json) + begin, new_size);
	free(*json);
	*json = new_json;
}

static void set_general_symbols(char** json, CONSTANTS* constants)
{
	char* key;
	uint16_t begin;
	unsigned int elem_number;
	unsigned int j;
	string* symbols_lst;
	unsigned long new_size;
	char* new_json;

	key = "\"SYMBOL\"";
	begin = strstr(*json, key) - *json;
	elem_number = 0;
	constants->general_symbols = (List*)malloc(sizeof(List));
	constants->general_symbols->length = 0;
	symbols_lst = (string*)malloc(sizeof(string));
	goto read_json;

read_elem:
	symbols_lst = (string*)realloc(symbols_lst, (1 + elem_number) * sizeof(string));
	j = 0;
	symbols_lst[elem_number] = (string)malloc(1);
	while ((*json)[begin] != '\"')
	{
		symbols_lst[elem_number] = (string)realloc(symbols_lst[elem_number], (1 + j));
		symbols_lst[elem_number][j] = (*json)[begin];
		(begin)++;
		j++;
	}

	symbols_lst[elem_number] = (string)realloc(symbols_lst[elem_number], (1 + j));
	symbols_lst[elem_number][j] = '\0';
	constants->general_symbols->length++;
	elem_number++;
	goto read_json;

read_json:
	while ((*json)[begin] != ']')
	{
		if ((*json)[begin] == '[')
		{
			(begin) += 2;
			goto read_elem;
		}

		else if ((*json)[begin] == ',')
		{
			(begin) += 3;
			goto read_elem;
		}

		else
		{
			(begin)++;
		}
	}

	begin += 3;
	constants->general_symbols->entries = symbols_lst;
	new_size = strlen(*json) - begin + 1;
	new_json = (char*)malloc(new_size);
	memcpy(new_json, (*json) + begin, new_size);
	free(*json);
	*json = new_json;
}

static void set_exchange_symbols(char** json, CONSTANTS* constants, string exchange)
{
	MAP* exchange_symbols_table;
	char* key;
	uint16_t begin;
	unsigned int elem_number;
	unsigned int j;
	string general_symbol;
	string exchange_symbol;
	SymbolProperty* property;
	unsigned long new_size;
	char* new_json;

	exchange_symbols_table = create_map();
	key = "\"SYMBOL\"";
	begin = strstr(*json, key) - *json;
	elem_number = 0;
	goto read_json;

read_elem:
	general_symbol = ((string*)constants->general_symbols->entries)[elem_number];
	exchange_symbol = (string)malloc(1);
	property = (SymbolProperty*)malloc(sizeof(SymbolProperty));
	j = 0;
	while ((*json)[begin] != '\"')
	{
		exchange_symbol = (string)realloc(exchange_symbol, (1 + j));
		exchange_symbol[j] = (*json)[begin];
		(begin)++;
		j++;
	}

	exchange_symbol = (string)realloc(exchange_symbol, (1 + j));
	exchange_symbol[j] = '\0';

	property->name = exchange_symbol;
	property->number = elem_number;
	map_set(exchange_symbols_table, general_symbol, property);

	elem_number++;
	goto read_json;

read_json:
	while ((*json)[begin] != ']')
	{
		if ((*json)[begin] == '[')
		{
			(begin) += 2;
			goto read_elem;
		}

		else if ((*json)[begin] == ',')
		{
			(begin) += 3;
			goto read_elem;
		}

		else
		{
			(begin)++;
		}
	}

	begin += 3;
	map_set(constants->symbols_table, exchange, exchange_symbols_table);
	new_size = strlen(*json) - begin + 1;
	new_json = (char*)malloc(new_size);
	memcpy(new_json, (*json) + begin, new_size);
	free(*json);
	*json = new_json;
}

static void set_exchange_symbols_ids(char** json, CONSTANTS* constants, string exchange)
{
	MAP* exchange_symbols_table;
	char* key;
	uint16_t begin;
	unsigned int elem_number;
	unsigned int j;
	string general_symbol;
	string exchange_symbol_id;
	SymbolProperty* property;
	unsigned long new_size;
	char* new_json;

	exchange_symbols_table = (MAP*)map_get_value(constants->symbols_table, exchange);
	key = "\"ID\"";
	begin = strstr(*json, key) - *json;
	elem_number = 0;
	goto read_json;

read_elem:
	general_symbol = ((string*)constants->general_symbols->entries)[elem_number];
	property = (SymbolProperty*)map_get_value(exchange_symbols_table, general_symbol);
	exchange_symbol_id = (string)malloc(1);
	j = 0;
	while ((*json)[begin] != '\"')
	{
		exchange_symbol_id = (string)realloc(exchange_symbol_id, (1 + j));
		exchange_symbol_id[j] = (*json)[begin];
		(begin)++;
		j++;
	}

	exchange_symbol_id = (string)realloc(exchange_symbol_id, (1 + j));
	exchange_symbol_id[j] = '\0';

	property->id = exchange_symbol_id;

	elem_number++;
	goto read_json;

read_json:
	while ((*json)[begin] != ']')
	{
		if ((*json)[begin] == '[')
		{
			(begin) += 2;
			goto read_elem;
		}

		else if ((*json)[begin] == ',')
		{
			(begin) += 3;
			goto read_elem;
		}

		else
		{
			(begin)++;
		}
	}

	begin += 3;
	new_size = strlen(*json) - begin + 1;
	new_json = (char*)malloc(new_size);
	memcpy(new_json, (*json) + begin, new_size);
	free(*json);
	*json = new_json;
}

static void set_string_array_constant_field(char** json, const char* key, size_t* counter, string** field)
{
    uint16_t begin = strstr(*json, key) - *json;
    size_t elem_number = 0;
    size_t j;
    if (counter != NULL) *counter = 0;
    *field = (char**)malloc(1 * sizeof(char*));
    goto read_json;
read_elem:
    *field = (char**)realloc(*field, (1 + elem_number) * sizeof(char*));
    j = 0;
    (*field)[elem_number] = (char*)malloc(1);
    while ((*json)[begin] != '\"')
    {
        (*field)[elem_number] = (char*)realloc((*field)[elem_number], (1 + j));
        (*field)[elem_number][j] = (*json)[begin];
        (begin)++;
        j++;
    }

    (*field)[elem_number] = (char*)realloc((*field)[elem_number], (1 + j));
    (*field)[elem_number][j] = '\0';
    if (counter != NULL) (*counter)++;
    elem_number++;
    goto read_json;
read_json:
    while ((*json)[begin] != ']')
    {
        if ((*json)[begin] == '[')
        {
            (begin) += 2;
            goto read_elem;
        }

        else if ((*json)[begin] == ',')
        {
            (begin) += 3;
            goto read_elem;
        }

        else
        {
            (begin)++;
        }
    }

    begin += 3;
    uint32_t new_size = strlen(*json) - begin + 1;
    char* new_json = (char*)malloc(new_size);
    memcpy(new_json, (*json) + begin, new_size);
    free(*json);
    *json = new_json;
}

static void set_tickers_margin(char** json, size_t** margin)
{
    uint16_t begin = strstr(*json, "\"MARGIN\"") - *json;
    size_t elem_number = 0;
    size_t j;
    *margin = (size_t*)malloc(1 * sizeof(size_t));
    goto read_json;
read_elem:
    *margin = (size_t*)realloc(*margin, (1 + elem_number) * sizeof(size_t));
    j = 0;
    char* elem = (char*)malloc(1);
    while ((*json)[begin] != '\"')
    {
        elem = (char*)realloc(elem, (1 + j));
        elem[j] = (*json)[begin];
        (begin)++;
        j++;
    }

    elem = (char*)realloc(elem, (1 + j));
    elem[j] = '\0';
    if (strcmp(elem, "True") == 0) (*margin)[elem_number] = 1;
    else if (strcmp(elem, "False") == 0 || strcmp(elem, "None") == 0) (*margin)[elem_number] = 0;
    free(elem);
    elem_number++;
    goto read_json;
read_json:
    while ((*json)[begin] != ']')
    {
        if ((*json)[begin] == '[')
        {
            (begin) += 2;
            goto read_elem;
        }

        else if ((*json)[begin] == ',')
        {
            (begin) += 3;
            goto read_elem;
        }

        else
        {
            (begin)++;
        }
    }

    begin += 3;
    uint32_t new_size = strlen(*json) - begin + 1;
    char* new_json = (char*)malloc(new_size);
    memcpy(new_json, (*json) + begin, new_size);
    free(*json);
    *json = new_json;
}

static void set_tickers_price_step(char** json, double** price_step)
{
    uint16_t begin = strstr(*json, "\"STEP\"") - *json;
    size_t elem_number = 0;
    size_t j;
    *price_step = (double*)malloc(1 * sizeof(long double));
    goto read_json;
read_elem:
    *price_step = (double*)realloc(*price_step, (1 + elem_number) * sizeof(double));
    j = 0;
    char* elem = (char*)malloc(1);
    while ((*json)[begin] != '\"')
    {
        elem = (char*)realloc(elem, (1 + j));
        elem[j] = (*json)[begin];
        (begin)++;
        j++;
    }

    elem = (char*)realloc(elem, (1 + j));
    elem[j] = '\0';
    (*price_step)[elem_number] = str2double(elem);
    free(elem);
    elem_number++;
    goto read_json;
read_json:
    while ((*json)[begin] != ']')
    {
        if ((*json)[begin] == '[')
        {
            (begin) += 2;
            goto read_elem;
        }

        else if ((*json)[begin] == ',')
        {
            (begin) += 3;
            goto read_elem;
        }

        else
        {
            (begin)++;
        }
    }

    begin += 3;
    uint32_t new_size = strlen(*json) - begin + 1;
    char* new_json = (char*)malloc(new_size);
    memcpy(new_json, (*json) + begin, new_size);
    free(*json);
    *json = new_json;
}

static void set_integer_constants_field(char** json, const char* key, size_t* field)
{
    uint16_t begin = strstr(*json, key) - *json;
    size_t j;

    while ((*json)[begin] != ':') {
        begin++;
    }

    begin++;

    j = 0;
    char* elem = (char*)malloc(1);
    while ((*json)[begin] != ',' && (*json)[begin] != '}') {
        elem = (char*)realloc(elem, (1 + j));
        elem[j] = (*json)[begin];
        (begin)++;
        j++;
    }

    elem = (char*)realloc(elem, (1 + j));
    elem[j] = '\0';

    *field = atoi(elem);
    free(elem);

    begin += 2;
    uint32_t new_size = strlen(*json) - begin + 1;
    char* new_json = (char*)malloc(new_size);
    memcpy(new_json, (*json) + begin, new_size);
    free(*json);
    *json = new_json;
}

static void set_double_constants_field(char** json, const char* key, double* field)
{
    uint16_t begin = strstr(*json, key) - *json;
    size_t j;

    while ((*json)[begin] != ':') {
        begin++;
    }

    begin++;

    j = 0;
    char* elem = (char*)malloc(1);
    while ((*json)[begin] != ',' && (*json)[begin] != '}') {
        elem = (char*)realloc(elem, (1 + j));
        elem[j] = (*json)[begin];
        (begin)++;
        j++;
    }

    elem = (char*)realloc(elem, (1 + j));
    elem[j] = '\0';

    *field = atof(elem);
    free(elem);

    begin += 2;
    uint32_t new_size = strlen(*json) - begin + 1;
    char* new_json = (char*)malloc(new_size);
    memcpy(new_json, (*json) + begin, new_size);
    free(*json);
    *json = new_json;
}

static void get_constants(SOCKET* listener, int* iRes, CONSTANTS* constants)
{
    char* msg = (char*)malloc(1);
    char buf[1];
    size_t iter = 0;

    do {
        *iRes = recv(*listener, buf, 1, 0);
        if (*iRes < 0)
        {
            printf("recv failed with error\n");
            close(*listener);
        }
        msg = (char*)realloc(msg, iter + 1);
        msg[iter] = buf[0];
        iter++;
    } while (buf[0] != '\0');

    unsigned int constants_size = atoi(msg);
    free(msg);

    if (constants_size) {
        char* json_constants = (char*)malloc(constants_size + 1);

        *iRes = recv(*listener, json_constants, constants_size + 1, 0);
        if (*iRes < 0)
        {
            printf("recv failed with error\n");
            close(*listener);
        }

        set_exchanges(&json_constants, constants);
        set_general_tickers(&json_constants, constants);
        constants->tickers_table = create_map();
        for (register unsigned int i = 0; i < constants->exchanges->length; i++) {
        	string exchange;

        	exchange = ((string*)constants->exchanges->entries)[i];
        	set_exchange_tickers(&json_constants, constants, exchange);
        	set_exchange_tickers_sub_names(&json_constants, constants, exchange);
        	set_exchange_tickers_margin(&json_constants, constants, exchange);
        	set_exchange_tickers_price_steps(&json_constants, constants, exchange);
        }

        set_general_symbols(&json_constants, constants);
        constants->symbols_table = create_map();
        for (register unsigned int i = 0; i < constants->exchanges->length; i++) {
			string exchange;

			exchange = ((string*)constants->exchanges->entries)[i];
			set_exchange_symbols(&json_constants, constants, exchange);
			set_exchange_symbols_ids(&json_constants, constants, exchange);
		}

//        for (size_t ex = 0; ex < constants->exchanges->length; ex++) {
//            Tickers* tickers = malloc(sizeof(Tickers));
//            set_string_array_constant_field(&json_constants, "\"TICKER\"", &(tickers->TICKERS_NUMBER), &(tickers->NAMES));
//            set_string_array_constant_field(&json_constants, "\"SUB_IDS\"", NULL, &(tickers->SUB_NAMES));
//            set_tickers_margin(&json_constants, &(tickers->MARGIN));
//            set_tickers_price_step(&json_constants, &(tickers->PRICE_STEPS));
//            map_set(constants->TICKERS, constants->EXCHANGES[ex], tickers);
//        }

//        constants->GENERAL_SYMBOLS = (GeneralSymbols*)malloc(sizeof(GeneralSymbols));
//        set_string_array_constant_field(&json_constants, "\"SYMBOL\"", &(constants->GENERAL_SYMBOLS->SYMBOLS_NUMBER),
//            &(constants->GENERAL_SYMBOLS->NAMES));
//
//        constants->SYMBOLS = create_map();
//
//        for (size_t ex = 0; ex < constants->EXCHANGES_NUMBER; ex++) {
//            Symbols* symbols = malloc(sizeof(Symbols));
//            set_string_array_constant_field(&json_constants, "\"SYMBOL\"", &(symbols->SYMBOLS_NUMBER), &(symbols->NAMES));
//            set_string_array_constant_field(&json_constants, "\"ID\"", NULL, &(symbols->IDS));
//            map_set(constants->SYMBOLS, constants->EXCHANGES[ex], symbols);
//        }

        set_integer_constants_field(&json_constants, "\"topN\"", &(constants->topN));
        set_double_constants_field(&json_constants, "\"epsilon\"", &(constants->epsilon));

        free(json_constants);
    }
}

static MAP* init_order_books(CONSTANTS* constants)
{
	MAP* ORDER_BOOKs;

    ORDER_BOOKs = create_map();
    for (register size_t a = 0; a < constants->exchanges->length; a++)
    {
    	MAP* EXCH_DICT;

        EXCH_DICT = create_map();

        for (register size_t b = 0; b < constants->general_tickers->length; b++)
        {
        	OrderBook* order_book;

        	order_book = (OrderBook*)malloc(sizeof(OrderBook));

        	order_book->asks = create_map();
        	order_book->bids = create_map();

        	for (register size_t n = 0; n < constants->topN; n++) {
        		char* pos_ask;
        		char* pos_bid;
        		Tranch* tranch_ask;
        		Tranch* tranch_bid;

        		pos_ask = zfill(n, 3);
        		pos_bid = zfill(n, 3);
        		tranch_ask = (Tranch*)malloc(sizeof(Tranch));
        		tranch_bid = (Tranch*)malloc(sizeof(Tranch));
        		tranch_ask->price = 0;
        		tranch_ask->size = 0;

        		tranch_bid->price = 0;
				tranch_bid->size = 0;

        		map_set(order_book->asks, pos_ask, tranch_ask);
        		map_set(order_book->bids, pos_bid, tranch_bid);

        	}

            map_set(EXCH_DICT, ((string*)constants->general_tickers->entries)[b], order_book);
        }

        map_set(ORDER_BOOKs, ((string*)constants->exchanges->entries)[a], EXCH_DICT);
    }

    return ORDER_BOOKs;
}

static void remove_order_books(MAP* order_books)
{
	char** exchanges;

	exchanges = map_get_entries(order_books);
	for (register size_t a = 0; a < order_books->length; a++) {
		MAP* EXCH_DICT;
		char** tickers;

		EXCH_DICT = (MAP*)map_get_value(order_books, exchanges[a]);
		tickers = map_get_entries(EXCH_DICT);
		for (register size_t b = 0; b < EXCH_DICT->length; b++) {
			OrderBook* order_book;
			char** tops_ask;
			char** tops_bid;

			order_book = (OrderBook*)map_get_value(EXCH_DICT, tickers[b]);
			tops_ask = map_get_entries(order_book->asks);
			tops_bid = map_get_entries(order_book->bids);
			for (register size_t n = 0; n < order_book->asks->length; n++) {
				free((Tranch*)map_get_value(order_book->asks, tops_ask[n]));
				free((Tranch*)map_get_value(order_book->bids, tops_bid[n]));

				free(tops_ask[n]);
				free(tops_bid[n]);
			}

			destroy_map(order_book->asks);
			destroy_map(order_book->bids);
			free(tops_ask);
			free(tops_bid);
			free(order_book);
		}

		destroy_map(EXCH_DICT);
		free(tickers);
	}

	destroy_map(order_books);
	free(exchanges);
}

static void init_tickers_mapping(CONSTANTS* constants)
{
//    for (register size_t i = 0; i < constants->EXCHANGES_NUMBER; i++) {
//        char* exchange = constants->EXCHANGES[i];
//
//        Tickers* tickers = (Tickers*)map_get_value(constants->TICKERS, exchange);
//        tickers->general2tickers = create_map();
//        tickers->tickers2general = create_map();
//        tickers->ticker2pricestep = create_map();
//        tickers->ticker2margin = create_map();
//
//        for (register int j = 0; j < constants->GENERAL->TICKERS_NUMBER; j++) {
//            char* general = constants->GENERAL->NAMES[j];
//
//            map_set(tickers->general2tickers, general, tickers->NAMES[j]);
//            if (strcmp(tickers->NAMES[j], "None") != 0) {
//                map_set(tickers->tickers2general, tickers->NAMES[j], general);
//                map_set(tickers->ticker2pricestep, tickers->NAMES[j], &(tickers->PRICE_STEPS[j]));
//                map_set(tickers->ticker2margin, tickers->NAMES[j], tickers->MARGIN[j]);
//            }
//        }
//    }
}

static void init_symbols_mapping(CONSTANTS* constants)
{
//    for (register size_t i = 0; i < constants->EXCHANGES_NUMBER; i++) {
//        char* exchange = constants->EXCHANGES[i];
//
//        Symbols* symbols = (Symbols*)map_get_value(constants->SYMBOLS, exchange);
//        symbols->general2symbols = create_map();
//        symbols->symbols2general = create_map();
//
//        for (register int j = 0; j < constants->GENERAL_SYMBOLS->SYMBOLS_NUMBER; j++) {
//            char* general = constants->GENERAL_SYMBOLS->NAMES[j];
//
//            map_set(symbols->general2symbols, general, symbols->NAMES[j]);
//            if (strcmp(symbols->NAMES[j], "None") != 0) {
//                map_set(symbols->symbols2general, symbols->NAMES[j], general);
//            }
//        }
//    }
}

static void init_couples(CONSTANTS* constants)
{
	unsigned long n;
	unsigned long couples_number;
	Couple** couples;

	constants->couples = (List*)malloc(sizeof(List));

	couples_number = 1;

	for (register size_t i = 1; i <= constants->exchanges->length; i++) {
		couples_number *= i;
	}

	couples_number /= 2;

	n = 0;
	couples = (Couple**)malloc(couples_number * sizeof(Couple*));
	for (register size_t i = 0; i < constants->exchanges->length; i++) {
		for (register unsigned long j = i + 1; j < constants->exchanges->length; j++) {
			Couple* couple;

			couple = (Couple*)malloc(sizeof(Couple));
			couple->left = ((string*)constants->exchanges->entries)[i];
			couple->right = ((string*)constants->exchanges->entries)[j];
			couple->value = (string)malloc(strlen(couple->left) + strlen(couple->right) + 2);
			strcpy(couple->value, couple->left);
			strcat(couple->value, "+");
			strcat(couple->value, couple->right);
			couples[n] = couple;
			n++;
		}
	}
	constants->couples->entries = couples;
	constants->couples->length = couples_number;
}

static MAP* init_arbitrage_structs(CONSTANTS* constants)
{
    MAP* ARBITRAGE;

	ARBITRAGE = create_map();

    for (register unsigned long i = 0; i < constants->couples->length; i++) {
        MAP* couple_table;
        Couple* couple;

    	couple_table = create_map();
    	couple = ((Couple**)constants->couples->entries)[i];

        for (register unsigned long j = 0; j < constants->general_tickers->length; j++) {
        	ArbitrageTable* table;
        	string ticker;

        	table = (ArbitrageTable*)malloc(sizeof(ArbitrageTable));
            table->result = NULL;
            table->maker = NULL;
            table->taker = NULL;
            table->new = 0;
            table->update = 0;
            table->delete = 0;

            ticker = ((string*)constants->general_tickers->entries)[j];

            map_set(couple_table, ticker, table);
        }

        map_set(ARBITRAGE, couple->value, couple_table);
    }

    return ARBITRAGE;
}

static void init_api_keys(CONSTANTS* constants)
{
    constants->API_KEYS = create_map();
    for (register size_t i = 0; i < constants->exchanges->length; i++) {
    	string exchange;

    	exchange = ((string*)constants->exchanges->entries)[i];

        if (strcmp(exchange, "BINANCE") == 0) {
            initialize_binance_api_keys(constants);
        }

        else if (strcmp(exchange, "HUOBI") == 0) {
            initialize_huobi_api_keys(constants);
        }

        else if (strcmp(exchange, "BITFINEX") == 0) {
            initialize_bitfinex_api_keys(constants);
        }
    }
}

static void init_commissions(CONSTANTS* constants)
{
	pthread_t tid[16];
    for (register size_t i = 0; i < constants->exchanges->length; i++) {
    	string exchange;

        exchange = ((string*)constants->exchanges->entries)[i];

        if (strcmp(exchange, "BINANCE") == 0) {
            printf("Initialize Binance commissions\n");
            CREATE_THREAD(initialize_binance_commissions, (void*)constants, tid[i]);
        }

        else if (strcmp(exchange, "HUOBI") == 0) {
            printf("Initialize Huobi commissions\n");
            CREATE_THREAD(initialize_huobi_commissions, (void*)constants, tid[i]);
        }

        else if (strcmp(exchange, "BITFINEX") == 0) {
            printf("Initialize Bitfinex commissions\n");
            CREATE_THREAD(initialize_bitfinex_commissions, (void*)constants, tid[i]);
        }
    }

//    pthread_join(tid[0], NULL);

    for (register size_t i = 0; i < constants->exchanges->length; i++) {
    	pthread_join(tid[i], NULL);
    }
}

static MAP* init_balances(CONSTANTS* constants)
{
	MAP* BALANCEs;
	pthread_t tid[16];

	BALANCEs = create_map();

    for (register size_t i = 0; i < constants->exchanges->length; i++) {
    	string exchange;
    	MAP* ex_dct;

    	exchange = ((string*)constants->exchanges->entries)[i];
        ex_dct = create_map();
        for (register unsigned long j = 0; j < constants->general_symbols->length; j++) {
        	string general_symbol;
        	Balance* balance;

        	general_symbol = ((string*)constants->general_symbols->entries)[j];

        	balance = (Balance*)malloc(sizeof(Balance));

        	balance->spot = (Wallet*)malloc(sizeof(Wallet));
        	balance->cross_margin = (Wallet*)malloc(sizeof(Wallet));

            balance->spot->free = 0.0;
            balance->spot->locked = 0.0;
            balance->spot->borrow = 0.0;
            balance->spot->maxBorrowable = 0.0;

            balance->cross_margin->free = 0.0;
            balance->cross_margin->locked = 0.0;
            balance->cross_margin->borrow = 0.0;
            balance->cross_margin->maxBorrowable = 0.0;

            map_set(ex_dct, general_symbol, balance);
        }
        BALANCE_REQUEST_PARAMS* params = (BALANCE_REQUEST_PARAMS*)malloc(sizeof(BALANCE_REQUEST_PARAMS));
        params->constants = constants;
        params->balance_state = ex_dct;
        if (strcmp(exchange, "BINANCE") == 0) {
            printf("Initialize Binance balance\n");
            CREATE_THREAD(initialize_binance_balance, (void*)params, tid[i]);
        }
        if (strcmp(exchange, "HUOBI") == 0) {
            printf("Initialize Huobi balance\n");
            CREATE_THREAD(initialize_huobi_balance, (void*)params, tid[i]);
        }
        if (strcmp(exchange, "BITFINEX") == 0) {
            printf("Initialize Bitfinex balance\n");
            CREATE_THREAD(initialize_bitfinex_balance, (void*)params, tid[i]);
        }
        map_set(BALANCEs, exchange, ex_dct);
    }

    for (register size_t i = 0; i < constants->exchanges->length; i++) {
		pthread_join(tid[i], NULL);
	}

    return BALANCEs;
}

static void run_market_websockets(CONSTANTS* constants, MAP* ORDER_BOOKs, CONFIG* config)
{
    for (register size_t i = 0; i < constants->exchanges->length; i++) {
    	string exchange;

    	exchange = ((string*)constants->exchanges->entries)[i];

        if (strcmp(exchange, "BINANCE") == 0) {
            run_binance_market_websockets(constants, (MAP*)map_get_value(ORDER_BOOKs, "BINANCE"), config);

        }

//        else if (strcmp(exchange, "HUOBI") == 0) {
//            run_huobi_market_websockets(constants, (MAP*)map_get_value(ORDER_BOOKs, "HUOBI"), config);
//        }
//
//        else if (strcmp(exchange, "BITFINEX") == 0) {
//            run_bitfinex_market_websockets(constants, (MAP*)map_get_value(ORDER_BOOKs, "BITFINEX"), config);
//        }
    }
}

int main(void)
{
//	struct addrinfo* res;
//	struct sockaddr_in* serv_addr;
//	TLS_SOCKET* tls_sock;
//
//	tls_sock = (TLS_SOCKET*)malloc(sizeof(TLS_SOCKET));
//
//	res = NULL;
//
//	getaddrinfo("api-pub.bitfinex.com", "443", 0, &res);
//
//	serv_addr = (struct sockaddr_in*)res->ai_addr;
//
//	tls_sock->sock = socket(AF_INET, SOCK_STREAM, 0);
//
//	int aaa = connect(tls_sock->sock, (struct sockaddr*)serv_addr, res->ai_addrlen);
//
//	freeaddrinfo(res);
//
//	free(serv_addr);

//	for (struct addrinfo* i = res->ai_next; i != NULL; i = i->ai_next){
//		free(i);
//	}
//
//	free(res);

//	char* a = "4e-7";
//	printf("%.10f\n", str2double(a));

    int iResult;
    SOCKET clientSocket = getCommandListener(&iResult);
    printf("Client is connected\n");

    char request[REQUEST_SIZE] = {0};

    CONSTANTS* constants = (CONSTANTS*)malloc(sizeof(CONSTANTS));
    GENERAL_STATES* states = (GENERAL_STATES*)malloc(sizeof(GENERAL_STATES));


#ifdef WIN32
    TLSLock = CreateMutex(NULL, 0, NULL);
    LogLock = CreateMutex(NULL, 0, NULL);
#elif __linux__
    //MUTEX TLSLock;
    pthread_mutex_init(&TLSLock, NULL);
    pthread_mutex_init(&LogLock, NULL);
#endif


    CONFIG* config = (CONFIG*)malloc(sizeof(CONFIG));
    LOG_QUEUE* log_queue = create_log_queue();

    while (1)
    {
        wait_for_bot_request(&clientSocket, &iResult, request);

        if (strcmp(request, "START") == 0) {
            printf("Start bot.\n");
            config->run = 1;
            start_logging(log_queue, config);
            log_msg(log_queue, "HELLO START BOT");
            get_constants(&clientSocket, &iResult, constants);
            config->topN = constants->topN;
            init_tickers_mapping(constants);
            init_symbols_mapping(constants);
            init_couples(constants);
            states->ORDER_BOOKs = init_order_books(constants);
            states->ARBITRAGES = init_arbitrage_structs(constants);
            //Tickers* tickers = (Tickers*)map_get_value(constants->TICKERS, "BINANCE");
            init_api_keys(constants);
            init_commissions(constants);
            states->BALANCEs = init_balances(constants);
            run_market_websockets(constants, states->ORDER_BOOKs, config);
//            run_bitfinex_market_websockets(constants, (MAP*)map_get_value(states->ORDER_BOOKs, "BITFINEX"), config);
//            run_binance_market_websockets(constants, (MAP*)map_get_value(states->ORDER_BOOKs, "BITFINEX"), config);
            sleep(10);
            printf("Start Detector\n");

//            while (1) {
//            	OrderBook* binance_btc_usdt;
//            	OrderBook* huobi_xtz_usdt;
//            	OrderBook* bitfinex_xmr_usdt;
//            	MAP* asks;
//            	MAP* bids;
//
//            	binance_btc_usdt = (OrderBook*)map_get_value((MAP*)map_get_value(states->ORDER_BOOKs, "BINANCE"), "BTC+USDT");
//            	huobi_xtz_usdt = (OrderBook*)map_get_value((MAP*)map_get_value(states->ORDER_BOOKs, "HUOBI"), "XTZ+USDT");
//            	bitfinex_xmr_usdt = (OrderBook*)map_get_value((MAP*)map_get_value(states->ORDER_BOOKs, "BITFINEX"), "BTC+USDT");

//            	asks = binance_btc_usdt->asks;
//				bids = binance_btc_usdt->bids;
//				printf("ASK:\n");
//            	for (register int i = 9; i >= 0; i--) {
//            		double price;
//            		double size;
//
//            		price = ((Tranch*)map_get_value(asks, zfill(i, 3)))->price;
//            		size = ((Tranch*)map_get_value(asks, zfill(i, 3)))->size;
//
//            		printf("%i --- %lf : %lf\n", i, price, size);
//            	}
//
//            	printf("BID:\n");
//            	for (register int i = 0; i < 10; i++) {
//					double price;
//					double size;
//
//					price = ((Tranch*)map_get_value(bids, zfill(i, 3)))->price;
//					size = ((Tranch*)map_get_value(bids, zfill(i, 3)))->size;
//
//					printf("%i --- %lf : %lf\n", i, price, size);
//				}
//
//            	asks = huobi_xtz_usdt->asks;
//				bids = huobi_xtz_usdt->bids;
//				printf("ASK:\n");
//				for (register int i = 9; i >= 0; i--) {
//					double price;
//					double size;
//
//					price = ((Tranch*)map_get_value(asks, zfill(i, 3)))->price;
//					size = ((Tranch*)map_get_value(asks, zfill(i, 3)))->size;
//
//					printf("%i --- %lf : %lf\n", i, price, size);
//				}
//
//				printf("BID:\n");
//				for (register int i = 0; i < 10; i++) {
//					double price;
//					double size;
//
//					price = ((Tranch*)map_get_value(bids, zfill(i, 3)))->price;
//					size = ((Tranch*)map_get_value(bids, zfill(i, 3)))->size;
//
//					printf("%i --- %lf : %lf\n", i, price, size);
//				}

//				asks = bitfinex_xmr_usdt->asks;
//				bids = bitfinex_xmr_usdt->bids;
//				printf("ASK:\n");
//				for (register int i = 9; i >= 0; i--) {
//					double price;
//					double size;
//
//					price = ((Tranch*)map_get_value(asks, zfill(i, 3)))->price;
//					size = ((Tranch*)map_get_value(asks, zfill(i, 3)))->size;
//
//					printf("%i --- %lf : %lf\n", i, price, size);
//				}
//
//				printf("BID:\n");
//				for (register int i = 0; i < 10; i++) {
//					double price;
//					double size;
//
//					price = ((Tranch*)map_get_value(bids, zfill(i, 3)))->price;
//					size = ((Tranch*)map_get_value(bids, zfill(i, 3)))->size;
//
//					printf("%i --- %lf : %lf\n", i, price, size);
//				}
//
//				sleep(1);
//            }
            //run_detector(constants, states, config, log_queue);

//            char* price = NULL;
//            char* prices[3] = {"0.0", "0.0", "0.0"};
////            printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA: %s", prices[1]);
//            while(1) {
//				price = (char*)map_get_value((MAP*)map_get_value((MAP*)map_get_value((MAP*)map_get_value((MAP*)
//					map_get_value(states->ORDER_BOOKs, "BITFINEX"), "BTC+USDT"), "prices"), "asks"), "6");
//				if (price){
//					if (strcmp(prices[0], price) != 0){
//						printf("BITFINEX -> %s\n", price);
//						prices[0] = price;
//					}
//				}
//				price = (char*)map_get_value((MAP*)map_get_value((MAP*)map_get_value((MAP*)map_get_value((MAP*)
//					map_get_value(states->ORDER_BOOKs, "BINANCE"), "BTC+USDT"), "prices"), "asks"), "6");
//				if (price){
//					if (strcmp(prices[1], price) != 0){
//						printf("BINANCE -> %s\n", price);
//						prices[1] = price;
//					}
//				}
//
//				price = (char*)map_get_value((MAP*)map_get_value((MAP*)map_get_value((MAP*)map_get_value((MAP*)
//					map_get_value(states->ORDER_BOOKs, "HUOBI"), "BTC+USDT"), "prices"), "asks"), "6");
//				if (price){
//					if (strcmp(prices[2], price) != 0){
//						printf("HUOBI -> %s\n", price);
//						prices[2] = price;
//					}
//				}
//			}
        }

        else if (strcmp(request, "STOP") == 0) {

            printf("Stop bot.\n");
            config->run = 0;
            free(config);
            remove_order_books(states->ORDER_BOOKs);
            destroy_map(states->ORDER_BOOKs);
//            for (register size_t i = 0; i < constants->EXCHANGES_NUMBER; i++) {
//                free(constants->EXCHANGES[i]);
//            }
//            free(constants->EXCHANGES);
//
//            for (register size_t i = 0; i < constants->GENERAL->TICKERS_NUMBER; i++) {
//                free(constants->GENERAL->NAMES[i]);
//            }
//            free(constants->GENERAL->NAMES);
//
//            destroy_map(constants->TICKERS);
            //string tickers[3] = {"BTC+ETH", "BTC+DOGE", "BTC+XRP"};

            /*for (size_t i = 0; i < 3; i++) {
                char* price = (char*)map_get_value((MAP*)map_get_value((MAP*)map_get_value((MAP*)map_get_value((MAP*)
                    map_get_value(states.ORDER_BOOKs, "BINANCE"), tickers[i]), "prices"), "asks"), "6");
                printf("%s\n", price);
            }*/

            break;
        }
    }

    // shutdown the connection since we're done
    iResult = shutdown(clientSocket, 1);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error\n");
        close(clientSocket);
        return 1;
    }

    // cleanup
    close(clientSocket);

    return 0;
}
