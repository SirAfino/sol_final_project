#include <stdlib.h>
#include <stdio.h>
#define CHECK_NEGATIVE(var, errorMessage) \
	if(var < 0) {perror(errorMessage); return -1;}
#define CHECK_NON_ZERO(var, errorMessage) \
	if(var != 0) {perror(errorMessage); return -1;}
#define CHECK_NULL(ptr, errorMessage) \
	if(ptr == NULL) {perror(errorMessage); return -1;}

typedef struct list_t{
	void* key;
	struct list_t* next;
}list;

list* ls_create()
{
	return NULL;
}

void ls_destroy(list* l)
{
	if(l == NULL)
	{
		return;
	}
	ls_destroy(l->next);
	free(l);
	return;
}

int ls_push(list** l, void* k)
{
	list* node = (list*) malloc(sizeof(list));
	CHECK_NULL(node, "malloc");
	node->key = k;
	node->next = NULL;
	if(*l == NULL)
	{
		*l = node;
	}
	else
	{
		list* ptr = *l;
		while(ptr->next != NULL)
		{
			ptr = ptr->next;
		}
		ptr->next = node;
	}
	return 0;
}

int ls_contains(list* l, void* k, int (*cmp)(void*, void*))
{
	while(l != NULL)
	{
		if(cmp(l->key, k) == 0)
		{
			return 1;
		}
		l = l->next;
	}
	return 0;
}

int ls_remove(list** l, void* k, int (*cmp)(void*, void*))
{
	if(*l == NULL)
	{
		return 0;
	}
	if(cmp((*l)->key, k) == 0)
	{
		list* temp = *l;
		*l = (*l)->next;
		free(temp->key);
		free(temp);
		return 1;
	}
	list* current = (*l)->next;
	list* previous = *l;
	while(current != NULL)
	{
		if(cmp(current->key, k) == 0)
		{
			previous->next = current->next;
			free(current->key);
			free(current);
			return 1;
		}
		previous = current;
		current = current->next;
	}
	return 0;
}