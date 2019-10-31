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

hashtable ht_create(int size, int (*hash)(void*), int (*compare)(void*, void*))
{
	hashtable hs;
	hs.buckets = (list**) malloc(size * sizeof(list*));
	for(int i=0;i<size;i++)
	{
		hs.buckets[i] = ls_create();
	}
	hs.size = size;
	hs.hash = hash;
	hs.compare = compare;
	hs.elements_number = 0;
	return hs;
}

void ht_destroy(hashtable hs)
{
	for(int i=0;i<hs.size;i++)
	{
		ls_destroy(hs.buckets[i]);
	}
	free(hs.buckets);
	return;
}

void ht_insert(hashtable* hs, void* element, void* toHash)
{
	int index = hs->hash(toHash) % hs->size;
	ls_push(&(hs->buckets[index]), element);
	hs->elements_number++;
	return;
}

int ht_contains(hashtable hs, void* element)
{
	int index = hs.hash(element) % hs.size;
	return ls_contains(hs.buckets[index], element, hs.compare);
}

void ht_remove(hashtable* hs, void* element)
{
	int index = hs->hash(element) % hs->size;
	hs->elements_number -= ls_remove(&(hs->buckets[index]), element, hs->compare);
	return;
}

void* ht_get(hashtable hs, void* element)
{
	void* result = NULL;
	int index = hs.hash(element) % hs.size;
	list* list_ptr = hs.buckets[index];
	while(list_ptr != NULL && result == NULL)
	{
		if(hs.compare(list_ptr->key, element) == 0)
		{
			result = list_ptr->key;
		}
		list_ptr = list_ptr->next;
	}
	return result;
}