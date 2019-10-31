#ifndef LIBHASHTABLE_H
#define LIBHASHTABLE_H
#include <stdlib.h>
#include <stdio.h>
#include "libList.h"

typedef struct hashtable_t {
	list** buckets;
	int size;
	int (*hash)(void*);
	int (*compare)(void*, void*);
	int elements_number;
} hashtable;

hashtable ht_create(int size, int (*hash)(void*), int (*compare)(void*, void*));
void ht_destroy(hashtable hs);
void ht_insert(hashtable* hs, void* element, void* toHash);
int ht_contains(hashtable hs, void* element);
void ht_remove(hashtable* hs, void* element);
void* ht_get(hashtable hs, void* element);
#endif