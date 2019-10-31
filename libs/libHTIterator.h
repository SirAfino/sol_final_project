#ifndef LIBHTITERATOR_H
#define LIBHTITERATOR_H
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

iterator it_create(hashtable* ht);
void* it_next(iterator* it);
int it_hasNext(iterator it);

#endif