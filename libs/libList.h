#ifndef LIBLIST_H
#define LIBLIST_H
typedef struct list_t{
	void* key;
	struct list_t* next;
}list;

list* ls_create();
void ls_destroy(list* l);
int ls_push(list** l, void* k);
int ls_contains(list* l, void* k, int (*cmp)(void*, void*));
int ls_remove(list** l, void* k, int (*cmp)(void*, void*));
#endif