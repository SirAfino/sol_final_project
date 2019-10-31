#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <signal.h>
#include "libs/libUtils.h"
#include "libs/libHashTable.h"
#include "libs/libHTIterator.h"
#include "libs/opCodes.h"
#define CHECK_NEGATIVE(var, errorMessage) \
	if(var < 0) {perror(errorMessage); exit(-1);}
#define CHECK_NON_ZERO(var, errorMessage) \
	if(var != 0) {perror(errorMessage); exit(-1);}
#define CHECK_NULL(ptr, errorMessage) \
	if(ptr == NULL) {perror(errorMessage); exit(-1);}

status server_status;
hashtable users_status;
pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;
hashtable connected_users;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

int active_threads;
pthread_cond_t threads_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t threads_mutex = PTHREAD_MUTEX_INITIALIZER;
int stop_flag;

void updateStatus(char* username, int operation, long size)
{
	pthread_mutex_lock(&status_mutex);
	server_status.objs_count += operation;
	server_status.objs_size += operation * size;
	status* user_status = ht_get(users_status, username);
	if(user_status != NULL)
	{
		user_status->objs_count += operation;
		user_status->objs_size += operation * size;
	}
	pthread_mutex_unlock(&status_mutex);
	return;
}

void printServerStatus()
{
	pthread_mutex_lock(&status_mutex);
	pthread_mutex_lock(&threads_mutex);
	printf("[SERVER]Server status\n");
	printf("	Connected users(%d):\n", active_threads);
	iterator users_iterator = it_create(&connected_users);
	while(it_hasNext(users_iterator) != 0)
	{
		printf("		%s\n", (char*) it_next(&users_iterator));
	}
	printf("	Total : %d obj/s (%ld byte/s)\n", server_status.objs_count, server_status.objs_size);
	iterator status_iterator = it_create(&users_status);
	while(it_hasNext(status_iterator) != 0)
	{
		status* s = (status*) it_next(&status_iterator);
		printf("		%s : %d obj/s (%ld byte/s)\n", s->username, s->objs_count, s->objs_size);
	}
	pthread_mutex_unlock(&threads_mutex);
	pthread_mutex_unlock(&status_mutex);
	return;
}

void incrementActiveThreads()
{
	pthread_mutex_lock(&threads_mutex);
	active_threads++;
	pthread_mutex_unlock(&threads_mutex);
	return;
}

void decrementActiveThreads()
{
	pthread_mutex_lock(&threads_mutex);
	active_threads--;
	if(active_threads <= 0)
	{
		pthread_cond_signal(&threads_cv);
	}
	pthread_mutex_unlock(&threads_mutex);
	return;
}

int isUsernameAvailable(char* username)
{
	pthread_mutex_lock(&users_mutex);
	int response = ht_contains(connected_users, username) - 1;
	pthread_mutex_unlock(&users_mutex);
	return response;
}

void createUserStatus(char* username)
{
	pthread_mutex_lock(&status_mutex);
	if(ht_contains(users_status, username) == 0)
	{
		status* user_status = (status*) malloc(sizeof(status));
		CHECK_NULL(user_status, "malloc");
		user_status->username = (char*) malloc((strlen(username) + 1) * sizeof(char));
		CHECK_NULL(user_status->username, "malloc");
		strcpy(user_status->username, username);
		user_status->objs_count = 0;
		user_status->objs_size = 0;
		ht_insert(&users_status, user_status, user_status->username);
	}
	pthread_mutex_unlock(&status_mutex);
	return;
}

void registerUser(char* username)
{
	pthread_mutex_lock(&users_mutex);
	char* buffer = (char*) malloc((strlen(username)+1) * sizeof(char));
	CHECK_NULL(buffer, "malloc");
	strcpy(buffer, username);
	ht_insert(&connected_users, (void*)buffer, (void*)buffer);
	pthread_mutex_unlock(&users_mutex);
	return;
}

void unregisterUser(char* username)
{
	pthread_mutex_lock(&users_mutex);
	ht_remove(&connected_users, (void*)username);
	pthread_mutex_unlock(&users_mutex);
	return;
}

int getRequestType(char* message, char** args)
{
	char* command = strtok_r(message, " \n\0", args);
	int response = UNKNOWN_COMMAND;
	if(strcmp(command, "REGISTER") == 0)
	{
		response = REGISTER_COMMAND;
	}
	else if(strcmp(command, "STORE") == 0)
	{
		response = STORE_COMMAND;
	}
	else if(strcmp(command, "RETRIEVE") == 0)
	{
		response = RETRIEVE_COMMAND;
	}
	else if(strcmp(command, "DELETE") == 0)
	{
		response = DELETE_COMMAND;
	}
	else if(strcmp(command, "LEAVE") == 0)
	{
		response = LEAVE_COMMAND;
	}
	return response;
}

void setupSocket(int* server_fd, char* socket_name)
{
	struct sockaddr_un server_address;
	int err;
	strcpy(server_address.sun_path, socket_name);
	server_address.sun_family = AF_UNIX;
	*server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	CHECK_NEGATIVE(*server_fd, "socket");
	err = bind(*server_fd, (struct sockaddr *)&server_address, sizeof(server_address));
	CHECK_NEGATIVE(err, "bind");
	err = listen(*server_fd, SOMAXCONN);
	CHECK_NEGATIVE(err, "listen");
	return;
}

void createUserDirectory(char* username)
{
	char* directory_path = getUserPath(username);
	int err = mkdir(directory_path, 0700);
	if(err != 0 && errno != EEXIST)
	{
		CHECK_NEGATIVE(err, "mkdir");
	}
	free(directory_path);
	return;
}

void register_handler(int client_fd, char* args, int* logged, char** username)
{
	if(*logged)
	{
		sendN(client_fd, "KO you are already registered\n");
		return;
	}
	if(strlen(args) == 0)
	{
		sendN(client_fd, "KO invalid username\n");
		return;
	}
	char* save_ptr;
	strcpy(*username, strtok_r(args, " \n\0", &save_ptr));
	if(isUsernameAvailable(*username) == 0)
	{
		sendN(client_fd, "KO username unavailable\n");
		return;
	}
	//printf("[LOG]REGISTER (user:%s)\n", *username);
	registerUser(*username);
	createUserStatus(*username);
	sendN(client_fd, "OK\n");
	createUserDirectory(*username);
	*logged = 1;
	return;
}

int checkLogged(int client_fd, int logged, int errorMessage)
{
	if(logged == 0 && errorMessage == 1)
	{
		sendN(client_fd, "KO unregistered user\n");
	}
	return logged;
}

int checkSize(char* buffer, long* size, char** save_ptr, int client_fd)
{
	*size = 0;
	if(strlen(buffer) > 0)
	{
		*size = strtol(strtok_r(buffer, "\n", save_ptr), NULL, 10);
	}
	if(*size <= 0)
	{
		sendN(client_fd, "KO invalid file size\n");
		return 0;
	}
	return 1;
}

int checkObject(int file_fd, int client_fd)
{
	if(file_fd == -1)
	{
		if(errno == EEXIST)
		{
			sendN(client_fd, "KO object already exists\n");
		}
		else
		{
			CHECK_NEGATIVE(file_fd, "open");
		}
		return 0;
	}
	return 1;
}

int checkFileName(char* buffer, int client_fd)
{
	if(strlen(buffer) == 0)
	{
		sendN(client_fd, "KO invalid filename\n");
		return 0;
	}
	return 1;
}

void store_handler(int client_fd, char* args, int logged, char* username)
{
	int write_flag;
	if(checkLogged(client_fd, logged, 1))
	{
		char* save_ptr;
		char* file_name = strtok_r(args, " ", &save_ptr);
		if(checkFileName(file_name, client_fd))
		{
			long file_size = 0;
			if(checkSize(save_ptr, &file_size, &save_ptr, client_fd))
			{
				char* file_path = getFilePath(username, file_name);
				int file_fd = open(file_path, O_WRONLY | O_CREAT | O_EXCL, 0700);
				write_flag = checkObject(file_fd, client_fd);
				char* buffer = (char*) malloc((file_size+2) * sizeof(char));
				CHECK_NULL(buffer, "malloc");
				strcpy(buffer, save_ptr);
				int bufferLength = strlen(buffer);
				file_size -= bufferLength;
				readN(client_fd, &(buffer[bufferLength]), file_size);
				if(write_flag)
				{
					writeN(file_fd, buffer, bufferLength + file_size);
					//printf("[LOG]STORE (user:%s) (obj:%s) (size:%ld)\n", username, file_name, file_size+bufferLength);
					updateStatus(username, 1, file_size+bufferLength);
					sendN(client_fd, "OK\n");
				}
				close(file_fd);
				free(buffer);
				free(file_path);
			}
		}
	}
	return;
}

void leave_handler(int client_fd, char* username, int* has_left)
{
	//printf("[LOG]LEAVE (user:%s)\n", username);
	sendN(client_fd, "OK\n");
	*has_left = 1;
	return;
}

void unkwnown_handler(int client_fd, char* username)
{
	//printf("[LOG]UNKWN (user:%s)\n", username);
	sendN(client_fd, "KO unkwnown command\n");
	return;
}

void retrieve_handler(int client_fd, char* args, int logged, char* username)
{
	if(checkLogged(client_fd, logged, 1))
	{
		char* save_ptr;
		char* file_name = strtok_r(args, " \n", &save_ptr);
		if(checkFileName(file_name, client_fd))
		{
			char* file_path = getFilePath(username, file_name);
			int file_fd = open(file_path, O_RDONLY);
			if(file_fd == -1 && errno == ENOENT)
			{
				sendN(client_fd, "KO object does not exist\n");
			}
			else
			{
				CHECK_NEGATIVE(file_fd, "open");
				struct stat file_stat;
				fstat(file_fd, &file_stat);
				long file_size = file_stat.st_size;
				char* buffer = (char*) malloc((file_size+16) * sizeof(char));
				CHECK_NULL(buffer, "malloc");
				strcpy(buffer, "OK ");
				sprintf(&(buffer[3]), "%ld\n", file_size);
				readN(file_fd, &(buffer[strlen(buffer)]), file_size);
				close(file_fd);
				sendN(client_fd, buffer);
				free(buffer);
				//printf("[LOG]RETRIEVE (user:%s) (obj:%s)\n", username, file_name);
			}
			free(file_path);
		}
	}
}

void delete_handler(int client_fd, char* args, int logged, char* username)
{
	if(checkLogged(client_fd, logged, 1))
	{
		char* save_ptr;
		char* file_name = strtok_r(args, " \n", &save_ptr);
		if(checkFileName(file_name, client_fd))
		{
			char* file_path = getFilePath(username, file_name);
			int file_fd = open(file_path, O_RDONLY);
			if(file_fd == -1 && errno == ENOENT)
			{
				sendN(client_fd, "KO object does not exist\n");
			}
			else
			{
				CHECK_NEGATIVE(file_fd, "open");
				struct stat file_stat;
				fstat(file_fd, &file_stat);
				long file_size = file_stat.st_size;
				close(file_fd);
				int err = remove(file_path);
				CHECK_NEGATIVE(err, "remove");
				sendN(client_fd, "OK\n");
				//printf("[LOG]DELETE (user:%s) (obj:%s)\n", username, file_name);
				updateStatus(username, -1, file_size);
			}
			free(file_path);
		}
	}
}

void* serverThread(void* arg)
{
	incrementActiveThreads();
	int err;
	int client_fd = *((int*) arg);
	free(arg);
	int read_size;
	int logged = 0;
	int request_type;
	char* args;
	fd_set set;
	fd_set rd_set;
	FD_ZERO(&set);
	FD_SET(client_fd, &set);
	struct timeval timeval_s;
	char* username = (char*) malloc(256 * sizeof(char));
	CHECK_NULL(username, "malloc");
	strcpy(username, NO_USERNAME);
	char* buffer = (char*) malloc(1025 * sizeof(char));
	CHECK_NULL(buffer, "malloc");
	//printf("[LOG]CONNECTED (client:%d)\n", client_fd);
	int has_left = 0;
	do{
		timeval_s.tv_sec = 0;
		timeval_s.tv_usec = 200000;
		rd_set = set;
		err = select(client_fd + 1, &rd_set, NULL, NULL, &timeval_s);
		CHECK_NEGATIVE(err, "select");
		if(FD_ISSET(client_fd, &rd_set))
		{
			memset(buffer, 0, 1025);
			read_size = read(client_fd, buffer, 1024);
			if(read_size > 0)
			{
				request_type = getRequestType(buffer, &args);
				switch(request_type)
				{
					case UNKNOWN_COMMAND: unkwnown_handler(client_fd, username); break;
					case REGISTER_COMMAND: register_handler(client_fd, args, &logged, &username); break;
					case STORE_COMMAND: store_handler(client_fd, args, logged, username); break;
					case RETRIEVE_COMMAND: retrieve_handler(client_fd, args, logged, username); break;
					case DELETE_COMMAND: delete_handler(client_fd, args, logged, username); break;
					case LEAVE_COMMAND: leave_handler(client_fd, username, &has_left); break;
				}
			}
			else
			{
				has_left = 1;
			}
		}
	}while(has_left == 0 && stop_flag == 0);
	//printf("[LOG]DISCONNECTED (client:%d) (user:%s)\n", client_fd, username);
	unregisterUser(username);
	free(buffer);
	free(username);
	decrementActiveThreads();
	return NULL;
}

void spawnDetachedThread(int client_fd)
{
	int* argument = (int*) malloc(sizeof(int));
	CHECK_NULL(argument, "malloc");
	*argument = client_fd;
	int err;
	pthread_t thread_fd;
	err = pthread_create(&thread_fd, NULL, &serverThread, (void*) argument);
	CHECK_NON_ZERO(err, "pthread_create");
	err = pthread_detach(thread_fd);
	CHECK_NON_ZERO(err, "pthread_detach");
	return;
}

static void stop_handler(int signum)
{
	stop_flag = 1;
	printf("[SERVER]Stop signal received\n");
	return;
}

static void usr1_handler(int signum)
{
	printServerStatus();
	return;
}

void saveServerStatus(char* file_name)
{
	pthread_mutex_lock(&status_mutex);
	int status_fd = open(file_name, O_WRONLY | O_CREAT, 0700);
	CHECK_NEGATIVE(status_fd, "open");
	write(status_fd, &(server_status.objs_count), sizeof(int));
	write(status_fd, &(server_status.objs_size), sizeof(long));
	write(status_fd, &(users_status.elements_number), sizeof(int));
	iterator status_iterator = it_create(&users_status);
	while(it_hasNext(status_iterator))
	{
		status* sts = (status*) it_next(&status_iterator);
		int username_length = strlen(sts->username);
		write(status_fd, &username_length, sizeof(int));
		write(status_fd, sts->username, username_length * sizeof(char));
		write(status_fd, &(sts->objs_count), sizeof(int));
		write(status_fd, &(sts->objs_size), sizeof(long));
		free(sts->username);
		free(sts);
	}
	close(status_fd);
	ht_destroy(users_status);
	pthread_mutex_unlock(&status_mutex);
	return;
}

void loadServerStatus(char* file_name)
{
	int status_fd = open(file_name, O_RDONLY);
	pthread_mutex_lock(&status_mutex);
	server_status.objs_count = 0;
	server_status.objs_size = 0;
	if(errno != ENOENT)
	{
		CHECK_NEGATIVE(status_fd, "open");
		int user_number;
		read(status_fd, &(server_status.objs_count), sizeof(int));
		read(status_fd, &(server_status.objs_size), sizeof(long));
		read(status_fd, &(user_number), sizeof(int));
		for(int i=0;i<user_number;i++)
		{
			int username_length;
			read(status_fd, &(username_length), sizeof(int));
			status* sts = (status*) malloc(sizeof(status));
			CHECK_NULL(sts, "malloc");
			sts->username = (char*) malloc((username_length+1) * sizeof(char));
			CHECK_NULL(sts->username, "malloc");
			memset(sts->username, 0, username_length+1);
			read(status_fd, sts->username, username_length * sizeof(char));
			read(status_fd, &(sts->objs_count), sizeof(int));
			read(status_fd, &(sts->objs_size), sizeof(long));
			ht_insert(&users_status, (void*) sts, (void*) sts->username);
		}
		close(status_fd);
	}
	pthread_mutex_unlock(&status_mutex);
	return;
}

void waitRunningThreads()
{
	pthread_mutex_lock(&threads_mutex);
	while(active_threads > 0)
	{
		pthread_cond_wait(&threads_cv, &threads_mutex);
	}
	pthread_mutex_unlock(&threads_mutex);
	return;
}

void safeServerTermination()
{
	saveServerStatus(STATUS_FILE);
	ht_destroy(connected_users);
	int err = remove(SOCK_NAME);
	CHECK_NEGATIVE(err, "remove");
	return;
}

void setSignalHandler(int signum, void (handler)(int))
{
	struct sigaction signal_handler_struct;
	memset(&signal_handler_struct, 0, sizeof(signal_handler_struct));
	signal_handler_struct.sa_handler = handler;
	int err = sigaction(signum, &signal_handler_struct, NULL);
	CHECK_NEGATIVE(err, "sigaction");
	return;
}

int main()
{
	printf("[SERVER]Starting server...\n");
	setSignalHandler(SIGINT, stop_handler);
	setSignalHandler(SIGQUIT, usr1_handler);
	setSignalHandler(SIGUSR1, usr1_handler);
	users_status = ht_create(100, stringHash, statusCompare);
	loadServerStatus(STATUS_FILE);
	stop_flag = 0;
	int client_fd;
	int server_fd;
	setupSocket(&server_fd, SOCK_NAME);
	connected_users = ht_create(100, stringHash, stringCompare);
	printf("[SERVER]Server is running!\n");
	while(stop_flag == 0)
	{
		errno = 0;
		client_fd = accept(server_fd, NULL, 0);
		if(stop_flag == 0 && errno != EINTR)
		{
			CHECK_NEGATIVE(client_fd, "accept");
			spawnDetachedThread(client_fd);
		}
	}
	waitRunningThreads();
	safeServerTermination();
	printf("[SERVER]All threads terminated\n");
	printf("[SERVER]Server terminated\n");
	exit(0);
}