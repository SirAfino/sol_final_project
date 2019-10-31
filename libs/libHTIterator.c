#include <stdlib.h>
#include <stdio.h>
#include "libList.h"
#include "libHashTable.h"

typedef struct iterator_t
{
	list* nextPtr;
	int index;
	hashtable* ht;
}iterator;

iterator it_create(hashtable* ht)
{
	iterator it;
	it.ht = ht;
	it.nextPtr = NULL;
	for(it.index = 0;it.index < ht->size && it.nextPtr == NULL;it.index++)
	{
		it.nextPtr = ht->buckets[it.index];
	}
	return it;
}

void* it_next(iterator* it)
{
	if(it->nextPtr == NULL)
	{
		return NULL;
	}
	void* result = (void*) (it->nextPtr)->key;
	if((it->nextPtr)->next != NULL)
	{
		it->nextPtr = (it->nextPtr)->next;
	}
	else
	{
		it->nextPtr = NULL;
		for(it->index++;it->index < it->ht->size && it->nextPtr == NULL;it->index++)
		{
			it->nextPtr = it->ht->buckets[it->index];
		}
	}
	return result;
}

int it_hasNext(iterator it)
{
	if(it.nextPtr == NULL)
	{
		return 0;
	}
	return 1;
}