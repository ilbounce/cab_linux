/*
 * Map.h
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */

#ifndef MAP_H_
#define MAP_H_

#ifdef WIN32
#pragma once
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	const char* key;
	void* value;
} mapEntry;

struct LinkedList {
	mapEntry* val;
	struct LinkedList* next;
};

typedef struct {
	struct LinkedList* entries_lst;
	size_t length;
} MAP;

MAP* create_map(void);
void destroy_map(MAP* map);
const char* map_set(MAP* map, const char* key, void* value);
const char* map_set_double(MAP* map, const char* key, void* value);
size_t map_check_key(MAP* map, const char* key);
size_t map_check_key_double(MAP* map, const char* key);
char* map_delete_key(MAP* map, const char* key);
char* map_delete_key_double(MAP* map, const char* key);
void* map_get_value(MAP* map, const char* key);
void* map_get_value_double(MAP* map, const char* key);
char** map_get_entries(MAP* map);

#endif /* MAP_H_ */
