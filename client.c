#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include "libs/libOS.h"
#include "libs/libUtils.h"

#define MAX_CONN_ATTEMPTS 5
#define CONN_ATTEMPTS_DELAY 3

char* username;

int attempt_connect(char* username, int attempts_no, int delay)
{
	int result = 0;
	int connection_attempts = 0;
	while((result = os_connect(username)) == 0 && connection_attempts < attempts_no)
	{
		connection_attempts++;
		sleep(delay);
	}
	return result;
}

void store_test()
{
	int ok_count = 0;
	char* data = (char*) malloc(100000 * sizeof(char));
	for(int i=0;i<100000;i++)
	{
		data[i] = '0' + (i%10);
	}
	char* name = (char*) malloc(10 * sizeof(char));
	int size = 100;
	for(int i=0;i<20;i++)
	{
		sprintf(name, "file%d", i);
		ok_count += os_store(name, data, size);
		size = min(size + 5300, 100000);
	}
	printf("%s;STORE;%d;20\n", username, ok_count);
	return;
}

int check_data(obj* object)
{
	if(object == NULL)
	{
		return 0;
	}
	int result = 1;
	for(int i=0;i<object->size && result == 1;i++)
	{
		if(((char*)object->data)[i] != '0' + (i%10))
		{
			result = 0;
		}
	}
	return result;
}

void retrieve_test()
{
	int ok_count = 0;
	obj* object;
	char* name = (char*) malloc(10 * sizeof(char));
	for(int i=0;i<20;i++)
	{
		sprintf(name, "file%d", i);
		object = os_retrieve(name);
		ok_count += check_data(object);
	}
	printf("%s;RETRIEVE;%d;20\n", username, ok_count);
	return;
}

void delete_test()
{
	int ok_count = 0;
	char* name = (char*) malloc(10 * sizeof(char));
	for(int i=0;i<20;i++)
	{
		sprintf(name, "file%d", i);
		ok_count += os_delete(name);
	}
	printf("%s;DELETE;%d;20\n", username, ok_count);
	return;
}

int main(int argc, char* argv[])
{
	username = argv[1];
	char* op_type = argv[2];
	if(attempt_connect(username, MAX_CONN_ATTEMPTS, CONN_ATTEMPTS_DELAY))
	{
		switch(atoi(op_type))
		{
			case 1: store_test(); break;
			case 2: retrieve_test(); break;
			case 3: delete_test(); break;
		}
	}
	else
	{
		printf("%s;CONNERR\n", username);
	}
	exit(0);
}