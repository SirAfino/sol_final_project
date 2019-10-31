#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define CHECK_NEGATIVE(var, errorMessage) \
	if(var < 0) {perror(errorMessage); exit(-1);}
#define CHECK_NON_ZERO(var, errorMessage) \
	if(var != 0) {perror(errorMessage); exit(-1);}
#define CHECK_NULL(ptr, errorMessage) \
	if(ptr == NULL) {perror(errorMessage); exit(-1);}

typedef struct status_t{
	char* username;
	int objs_count;
	long objs_size;
}status;


int stringCompare(void* a, void* b)
{
	return strcmp((char*) a, (char*)b);
}

void writeN(int fd, char* buffer, int size)
{
	int sent;
	int totalSent = 0;
	while(totalSent < size)
	{
		sent = write(fd, &(buffer[totalSent]), size-totalSent);
		CHECK_NEGATIVE(sent, "write");
		totalSent += sent;
	}
	return;
}

void readN(int fd, char* buffer, int size)
{
	int received;
	int totalReceived = 0;
	while(totalReceived < size)
	{
		received = read(fd, &(buffer[totalReceived]), size-totalReceived);
		totalReceived += received;
	}
	buffer[totalReceived] = '\0';
	return;
}

char* getUserPath(char* username)
{
	char* path = (char*) malloc(256 * sizeof(char));
	CHECK_NULL(path, "malloc");
	snprintf(path, 256, "./data/%s", username);
	return path;
}

char* getFilePath(char* username, char* filename)
{
	char* path = (char*) malloc(256 * sizeof(char));
	CHECK_NULL(path, "malloc");
	snprintf(path, 256, "./data/%s/%s.dat", username, filename);
	return path;
}

void sendN(int fd, char* buffer)
{
	writeN(fd, buffer, strlen(buffer));
}

int stringHash(void* str)
{
	int hash = 0;
	for(int i=0;i<strlen((char*)str);i++)
	{
		hash += ((char*)str)[i];
	}
	return hash;
}

int statusHash(void* sts)
{
	status* s = (status*) sts;
	return stringHash((void*)(s->username));
}

int statusCompare(void* a, void* b)
{
	status* sts = (status*) a;
	return strcmp(sts->username, (char*)b);
}

int min(int a, int b)
{
	if(a < b)
	{
		return a;
	}
	return b;
}