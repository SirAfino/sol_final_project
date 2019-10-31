#ifndef LIBOS_H
#define LIBOS_H

typedef struct obj_t{
	char* name;
	void* data;
	size_t size;
}obj;

extern int fd;
extern int error_no;

int os_connect(char* name);
int os_store(char* name, void* block, size_t len);
void* os_retrieve(char* name);
int os_delete(char* name);
int os_disconnect();

#endif