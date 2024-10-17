/*
 * Map.c
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */

#include "Map.h"

static const char* map_set_entry(struct LinkedList** entries, const char* key, void* value, size_t* len);
static void map_set_entry_first(struct LinkedList** entries, const char* key, void* value);
static void map_set_entry_between(struct LinkedList** left, struct LinkedList** right, const char* key, void* value);
static void map_set_entry_last(struct LinkedList** entries, const char* key, void* value);
static const char* map_set_entry_double(struct LinkedList** entries, const char* key, void* value, size_t* len);

MAP* create_map(void)
{
	MAP* map = malloc(sizeof(MAP));
	map->length = 0;
	return map;
}

static void map_set_entry_first(struct LinkedList** entries, const char* key, void* value)
{
	struct LinkedList* head = malloc(sizeof(struct LinkedList));
	head->val = malloc(sizeof(mapEntry));
	head->next = malloc(sizeof(struct LinkedList));
	head->val->key = key;
	head->val->value = value;
	head->next = *entries;
	*entries = head;
}

static void map_set_entry_between(struct LinkedList** left, struct LinkedList** right, const char* key, void* value)
{
	struct LinkedList* node = (struct LinkedList*)malloc(sizeof(struct LinkedList));
	node->val = (mapEntry*)malloc(sizeof(mapEntry));
	node->next = (struct LinkedList*)malloc(sizeof(struct LinkedList));
	node->val->key = key;
	node->val->value = value;
	node->next = (*right);
	(*left)->next = node;
}

static void map_set_entry_last(struct LinkedList** entries, const char* key, void* value)
{
	(*entries)->next = (struct LinkedList*)malloc(sizeof(struct LinkedList));
	(*entries)->next->val = (mapEntry*)malloc(sizeof(mapEntry));
	(*entries)->next->val->key = key;
	(*entries)->next->val->value = value;
	(*entries)->next->next = NULL;
}

static const char* map_set_entry(struct LinkedList** entries, const char* key, void* value, size_t* len)
{
	if (*len == 0)
	{
		*entries = malloc(sizeof(struct LinkedList));
		(*entries)->val = malloc(sizeof(mapEntry));
		(*entries)->next = NULL;
		(*entries)->val->key = key;
		(*entries)->val->value = value;
		(*len)++;
	}

	else
	{
		struct LinkedList* start = *entries;
		struct LinkedList* last = NULL;
		struct LinkedList* prev = start;

		do
		{
			struct LinkedList* current = start;
			struct LinkedList* nxt = start->next;

			while (nxt != last)
			{
				nxt = nxt->next;
				if (nxt != last) {
					prev = current;
					current = current->next;
					nxt = nxt->next;
				}
			}

			if (strcmp(current->val->key, key) == 0) {
				current->val->value = value;
				break;
			}

			else if (strcmp(current->val->key, key) < 0) {
				prev = current;
				start = current->next;

				if (start == NULL) {
					map_set_entry_last(&prev, key, value);
					(*len)++;
					break;
					/*last = current;
					start = current;*/
				}

				else if (strcmp(start->val->key, key) > 0)
				{
					map_set_entry_between(&prev, &start, key, value);
					(*len)++;
					break;
					/*last = current;
					start = current;*/
				}
			}

			else {
				if (prev == current->next || prev == current) {
					map_set_entry_first(entries, key, value);
					(*len)++;
					break;
					//start = current;
				}
				else if (strcmp(prev->val->key, key) < 0) {
					map_set_entry_between(&prev, &current, key, value);
					(*len)++;
					break;
					//start = current;
				}
				last = current;
			}
		} while (last == NULL || last != start);
	}

	return key;
}

static const char* map_set_entry_double(struct LinkedList** entries, const char* key, void* value, size_t* len)
{
	if (*len == 0)
	{
		*entries = malloc(sizeof(struct LinkedList));
		(*entries)->val = malloc(sizeof(mapEntry));
		(*entries)->next = NULL;
		(*entries)->val->key = key;
		(*entries)->val->value = value;
		(*len)++;
	}

	else
	{
		struct LinkedList* start = *entries;
		struct LinkedList* last = NULL;
		struct LinkedList* prev = start;

		do
		{
			struct LinkedList* current = start;
			struct LinkedList* nxt = start->next;

			while (nxt != last)
			{
				nxt = nxt->next;
				if (nxt != last) {
					prev = current;
					current = current->next;
					nxt = nxt->next;
				}
			}

			if (atof(current->val->key) == atof(key)) {
				current->val->value = value;
				break;
			}

			else if (atof(current->val->key) < atof(key)) {
				prev = current;
				start = current->next;

				if (start == NULL) {
					map_set_entry_last(&prev, key, value);
					(*len)++;
					break;
					/*last = current;
					start = current;*/
				}

				else if (atof(start->val->key) > atof(key))
				{
					map_set_entry_between(&prev, &start, key, value);
					(*len)++;
					break;
					/*last = current;
					start = current;*/
				}
			}

			else {
				if (prev == current->next || prev == current) {
					map_set_entry_first(entries, key, value);
					(*len)++;
					break;
					//start = current;
				}
				else if (atof(prev->val->key) < atof(key)) {
					map_set_entry_between(&prev, &current, key, value);
					(*len)++;
					break;
					//start = current;
				}
				last = current;
			}
		} while (last == NULL || last != start);
	}

	return key;
}

const char* map_set(MAP* map, const char* key, void* value)
{
	if (key == NULL) {
		return NULL;
	}
	return map_set_entry(&map->entries_lst, key, value, &map->length);
}

const char* map_set_double(MAP* map, const char* key, void* value)
{
	if (key == NULL){
		return NULL;
	}

	return map_set_entry_double(&map->entries_lst, key, value, &map->length);
}

void* map_get_value(MAP* map, const char* key)
{
	struct LinkedList* start = map->entries_lst;
	struct LinkedList* last = NULL;

	if (map->length == 0) {
		return NULL;
	}
	if (key == NULL) {
		return NULL;
	}

	do
	{
		struct LinkedList* current = start;
		struct LinkedList* nxt = start->next;

		while (nxt != last)
		{
			nxt = nxt->next;
			if (nxt != last) {
				current = current->next;
				nxt = nxt->next;
			}
		}

		if (strcmp(current->val->key, key) == 0) {
			return current->val->value;
		}

		else if (strcmp(current->val->key, key) < 0) {
			start = current->next;
		}

		else {
			last = current;
		}
	} while ((last == NULL || last != start) & (start != NULL));

	return NULL;
}

void* map_get_value_double(MAP* map, const char* key)
{
	struct LinkedList* start = map->entries_lst;
	struct LinkedList* last = NULL;

	if (map->length == 0) {
		return NULL;
	}
	if (key == NULL) {
		return NULL;
	}

	do
	{
		struct LinkedList* current = start;
		struct LinkedList* nxt = start->next;

		while (nxt != last)
		{
			nxt = nxt->next;
			if (nxt != last) {
				current = current->next;
				nxt = nxt->next;
			}
		}

		if (atof(current->val->key) == atof(key)) {
			return current->val->value;
		}

		else if (atof(current->val->key) < atof(key)){
			start = current->next;
		}

		else {
			last = current;
		}
	} while ((last == NULL || last != start) & (start != NULL));

	return NULL;
}

size_t map_check_key(MAP* map, const char* key)
{
	if (map_get_value(map, key) == NULL) {
		return 0;
	}

	else return 1;
}

size_t map_check_key_double(MAP* map, const char* key)
{
	if (map_get_value_double(map, key) == NULL){
		return 0;
	}

	else return 1;
}

char* map_delete_key(MAP* map, const char* key)
{
	char* ret_key = "\0";
	if (map_check_key(map, key) == 0) {
		return "\0";
	}
	if (strcmp(map->entries_lst->val->key, key) == 0)
	{
		struct LinkedList* tmp = map->entries_lst;
		if (tmp->next == NULL)
		{
			ret_key = tmp->val->key;
			free(tmp->val);
			free(map->entries_lst);
		}
		else
		{
			map->entries_lst = map->entries_lst->next;
			ret_key = tmp->val->key;
			free(tmp->val);
			free(tmp);
		}

		map->length--;
		return ret_key;
	}

	else
	{
		struct LinkedList* start = map->entries_lst->next;
		struct LinkedList* last = NULL;
		struct LinkedList* prev = map->entries_lst;
		do
		{
			struct LinkedList* current = start;
			struct LinkedList* nxt = start->next;

			while (nxt != last)
			{
				nxt = nxt->next;
				if (nxt != last) {
					prev = current;
					current = current->next;
					nxt = nxt->next;
				}
			}
			if ((prev == current) || (prev == NULL)) {
				struct LinkedList* tmp = map->entries_lst;

				while (tmp != current) {
					prev = tmp;
					tmp = tmp->next;
				}
			}

			if (current->val->key == NULL) {
				printf("%d\n", map->length);
				//for ()
			}

			if (current->val->key != NULL) {
				if (strcmp(current->val->key, key) == 0) {
					/*if (last != NULL) {
						printf("%s\n", prev->next->val->key);
					}*/
					struct LinkedList* tmp = current;
					prev->next = current->next;
					ret_key = tmp->val->key;
					free(tmp->val);
					free(tmp);
					map->length--;
					break;
				}

				else if (strcmp(current->val->key, key) < 0) {
					prev = current;
					start = current->next;
				}

				else {
					last = current->next;
				}
			}
			else {
				return NULL;
			}
		} while ((last == NULL || last != start) & (start != NULL));

		return ret_key;
	}
}

char* map_delete_key_double(MAP* map, const char* key)
{
	char* ret_key = "\0";
	if (map_check_key_double(map, key) == 0) {
		return "\0";
	}

	if (atof(map->entries_lst->val->key) == atof(key))
	{
		struct LinkedList* tmp = map->entries_lst;
		if (tmp->next == NULL)
		{
			ret_key = tmp->val->key;
			free(tmp->val);
			free(map->entries_lst);
		}
		else
		{
			map->entries_lst = map->entries_lst->next;
			ret_key = tmp->val->key;
			free(tmp->val);
			free(tmp);
		}

		map->length--;
		return ret_key;
	}

	else
	{
		struct LinkedList* start = map->entries_lst->next;
		struct LinkedList* last = NULL;
		struct LinkedList* prev = map->entries_lst;
		do
		{
			struct LinkedList* current = start;
			struct LinkedList* nxt = start->next;

			while (nxt != last)
			{
				nxt = nxt->next;
				if (nxt != last) {
					prev = current;
					current = current->next;
					nxt = nxt->next;
				}
			}
			if ((prev == current) || (prev == NULL)) {
				struct LinkedList* tmp = map->entries_lst;

				while (tmp != current) {
					prev = tmp;
					tmp = tmp->next;
				}
			}

			if (current->val->key == NULL) {
				printf("%d\n", map->length);
			}

			if (current->val->key != NULL) {
				if (atof(current->val->key) == atof(key)) {
					struct LinkedList* tmp = current;
					prev->next = current->next;
					ret_key = tmp->val->key;
					free(tmp->val);
					free(tmp);
					map->length--;
					break;
				}

				else if (atof(current->val->key) < atof(key)){
					prev = current;
					start = current->next;
				}

				else {
					last = current->next;
				}
			}
			else {
				return NULL;
			}
		} while ((last == NULL || last != start) & (start != NULL));

		return ret_key;
	}
}

char** map_get_entries(MAP* map)
{
	if (map->length == 0) {
		return NULL;
	}
	char** output = (char**)malloc(map->length * sizeof(char*));
	struct LinkedList* current = map->entries_lst;
	for (register int i = 0; i < map->length; i++) {
		output[i] = current->val->key;
		current = current->next;
	}

	return output;
}

void destroy_map(MAP* map) {
	struct LinkedList* tmp;
	if (map->length == 0) {
		free(map);
		return;
	}
	while (map->entries_lst != NULL) {
		tmp = map->entries_lst;
		map->entries_lst = map->entries_lst->next;
		free(tmp->val);
		free(tmp);
	}

	free(map);
}

