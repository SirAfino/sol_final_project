#ifndef LIBUTILS_H
#define LIBUTILS_H

typedef struct status_t{
	char* username;
	int objs_count;
	long objs_size;
}status;

int stringCompare(void* a, void* b);
void writeN(int fd, char* buffer, int size);
void readN(int fd, char* buffer, int size);
char* getUserPath(char* username);
char* getFilePath(char* username, char* filename);
void sendN(int fd, char* buffer);
int stringHash(void* str);
int statusHash(void* sts);
int statusCompare(void* a, void* b);
int min(int a, int b);

#endif