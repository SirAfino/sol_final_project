#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "opCodes.h"
#include "libUtils.h"
#define CHECK_SOCKET(return_value) \
	if(fd < 0) { error_no = SOCKET_ERROR; return return_value; }
#define CHECK_MEM_ALLOC(ptr, return_value) \
	if(ptr == NULL) { error_no = MEMORY_ERROR; return return_value; }
#define CHECK_RESPONSE(ptr, ok_response, error_type, return_value) \
	if(strcmp(ptr, ok_response) != 0) { error_no = error_type; return return_value; }
#define RESET_ERROR_NO() \
	error_no = 0;

typedef struct obj_t{
	char* name;
	void* data;
	size_t size;
}obj;

int fd = -1;
int error_no = 0;

int os_connect(char* name)
{
	RESET_ERROR_NO();
	int err;
	struct sockaddr_un socket_address;
	strcpy(socket_address.sun_path, SOCK_NAME);
	socket_address.sun_family = AF_UNIX;
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	CHECK_SOCKET(0);
	err = connect(fd, (struct sockaddr*) &socket_address, sizeof(socket_address));
	if(err < 0)
	{
		error_no = SERVER_ERROR;
		return 0;
	}
	char* request = (char*) malloc(sizeof(char) * 1024);
	CHECK_MEM_ALLOC(request, 0);
	snprintf(request, 1024, "REGISTER %s\n", name);
	sendN(fd, request);
	char* buffer = (char*) malloc(sizeof(char) * 1024);
	CHECK_MEM_ALLOC(buffer, 0);
	memset(buffer, 0, sizeof(char) * 1024);
	read(fd, buffer, 1024);
	CHECK_RESPONSE(buffer, "OK\n", REGISTER_ERROR, 0);
	return 1;
}

int os_store(char* name, void* block, size_t len)
{
	RESET_ERROR_NO();
	CHECK_SOCKET(0);
	char* request = (char*) malloc(sizeof(char) * 1024);
	CHECK_MEM_ALLOC(request, 0);
	snprintf(request, 1024, "STORE %s %ld\n", name, len);
	sendN(fd, request);
	writeN(fd, (char*) block, len);
	char* buffer = (char*) malloc(sizeof(char) * 1024);
	CHECK_MEM_ALLOC(buffer, 0);
	read(fd, buffer, 1024);
	CHECK_RESPONSE(buffer, "OK\n", STORE_ERROR, 0);
	return 1;
}

void* os_retrieve(char* name)
{
	RESET_ERROR_NO();
	CHECK_SOCKET(NULL);
	obj* object = (obj*) malloc(sizeof(obj));
	CHECK_MEM_ALLOC(object, NULL);
	char* request = (char*) malloc(sizeof(char) * 1024);
	CHECK_MEM_ALLOC(request, NULL);
	snprintf(request, 1024, "RETRIEVE %s", name);
	sendN(fd, request);
	char* buffer = (char*) malloc(sizeof(char) * 1024);
	CHECK_MEM_ALLOC(buffer, NULL);
	long read_bytes = read(fd, buffer, 1024);
	char* save_ptr;
	char* result = strtok_r(buffer, " \n\0", &save_ptr);
	CHECK_RESPONSE(result, "OK", RETRIEVE_ERROR, NULL);
	object->name = (char*) malloc((strlen(name)+1) * sizeof(char));
	CHECK_MEM_ALLOC(object->name, NULL);
	strcpy(object->name, name);
	object->size = strtol(strtok_r(NULL, "\n", &save_ptr), NULL, 10);
	read_bytes -= save_ptr - buffer;
	object->data = (void*) malloc(object->size+1);
	CHECK_MEM_ALLOC(object->data, NULL);
	memset(object->data, 0, object->size+1);
	memcpy(object->data, save_ptr, read_bytes);
	readN(fd, &(((char*)object->data)[read_bytes]), object->size - read_bytes);
	return (void*) object;
}

int os_delete(char* name)
{
	RESET_ERROR_NO();
	CHECK_SOCKET(0);
	char* request = (char*) malloc(sizeof(char) * 1024);
	CHECK_MEM_ALLOC(request, 0);
	snprintf(request, 1024, "DELETE %s", name);
	sendN(fd, request);
	char* buffer = (char*) malloc(sizeof(char) * 1024);
	CHECK_MEM_ALLOC(buffer, 0);
	read(fd, buffer, 1024);
	CHECK_RESPONSE(buffer, "OK\n", DELETE_ERROR, 0);
	return 1;
}

int os_disconnect()
{
	RESET_ERROR_NO();
	CHECK_SOCKET(0);
	char* request = (char*) malloc(sizeof(char) * 1024);
	CHECK_MEM_ALLOC(request, 0);
	snprintf(request, 1024, "LEAVE");
	sendN(fd, request);
	char* buffer = (char*) malloc(sizeof(char) * 1024);
	CHECK_MEM_ALLOC(buffer, 0);
	read(fd, buffer, 1024);
	CHECK_RESPONSE(buffer, "OK\n", LEAVE_ERROR, 0);
	return 1;
}