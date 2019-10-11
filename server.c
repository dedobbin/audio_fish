#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include "server.h"

static int _server_sock;
static int _client_sock = 0;

static char _data[65535];
static int _data_size = 0;
static int _terminate = 0;
static int _port = 0;

static pthread_mutex_t _data_ready_mutex;
static pthread_cond_t _data_ready_condition;

static pthread_t _transfer_thread = 0, _connection_thread = 0;

void* _send(void*param)
{
	int result = 0;
	do{
		pthread_mutex_lock(&_data_ready_mutex);
		if (_data_size <= 0){
			//printf("Server: Waiting for data\n");
			struct timespec timeout;
			timespec_get(&timeout, TIME_UTC);
			timeout.tv_sec += SERVER_SEND_THREAD_SLEEP_S;
			result = pthread_cond_timedwait(&_data_ready_condition, &_data_ready_mutex, &timeout); 
		}
		if (ETIMEDOUT != result){
			int sent_bytes = send(_client_sock, _data, _data_size, 0);
			if (!sent_bytes){
				_terminate = 1;
			}
			_data_size = 0;
		} else {
			//printf("Server: time out waiting for data\n");
		}
		pthread_mutex_unlock(&_data_ready_mutex);
	} while(!_terminate);
	//printf("Server: stopped sending\n");
	return NULL;
}

void* _wait_for_connection(void* param)
{
    struct sockaddr_in server;

    if ((_server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        return NULL;
    }

	struct timeval timeout;      
    timeout.tv_sec = SERVER_WAITING_FOR_CONNECTION_TIMEOUT_S;
    timeout.tv_usec = 0;

	if (setsockopt (_server_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
		fprintf(stderr, "Server: Failed setting sockoptions %s\n", strerror(errno));		
		return NULL;
	}

	if (setsockopt(_server_sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
		fprintf(stderr, "Server: Failed setting sockoptions %s\n", strerror(errno));		
		return NULL;
	}

    bzero((char *) &server, sizeof(server));
    server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(_port);
    if (bind(_server_sock, (struct sockaddr *)&server , sizeof(server)) < 0){
        fprintf(stderr, "Server: socket binding failed: %s\n", strerror(errno));		
		return NULL;
    }

    listen(_server_sock , SERVER_MAX_CLIENTS);

    struct sockaddr_in client;
    int clilen = sizeof(client);

	//printf("Server: Waiting for connection, port: %d\n", _port);
    int client_sock = accept(_server_sock, (struct sockaddr *)&client, &clilen);
	int result = 1;
    if (client_sock < 0){
		fprintf(stderr, "Server: Failed to get connection: %s\n", strerror(errno));
    } else {	
		_client_sock = client_sock;
		if (pthread_create(&_transfer_thread, NULL, &_send, NULL) != 0){
			fprintf(stderr, "Server: Failed creating transfer thread: %s\n", strerror(errno));	
		}
		//printf("Server: got connection\n");
	}
	return NULL;
}

int server_start(int port)
{
	_port = port;;
	pthread_mutex_init(&_data_ready_mutex, NULL);
	pthread_cond_init(&_data_ready_condition, NULL);
	if (pthread_create(&_connection_thread, NULL, &_wait_for_connection, NULL) != 0){
		fprintf(stderr, "Server: Failed creating connection thread: %s\n", strerror(errno));	
			
		return -3;	
	}	
	return 1;
}

void server_send(const char const* data, int data_size)
{
	pthread_mutex_lock(&_data_ready_mutex);
	memcpy(&_data[_data_size], data, data_size);
	_data_size += data_size;
	pthread_cond_signal(&_data_ready_condition);
	pthread_mutex_unlock(&_data_ready_mutex);
}

void server_close(){
	_terminate = 1;
	pthread_join(_connection_thread, NULL);
	pthread_join(_transfer_thread, NULL);
	if (_client_sock > 0){
		close(_client_sock);
	}
}