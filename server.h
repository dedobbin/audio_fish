#ifndef __SERVER_H__
#define __SERVER_H__

/**
server_start(1231);
server_send("stuff",5);
server_close();
 **/

//TODO: support multiple clients
#define SERVER_MAX_CLIENTS 1
#define SERVER_WAITING_FOR_CONNECTION_TIMEOUT_S 30
#define SERVER_SEND_THREAD_SLEEP_S 10

/* New thread waits for connection until SERVER_TIMEOUT_S */
int server_start(int port);
void server_send(const char const* data, int data_len);
/* Will block until until waiting for connection times out (SERVER_TIMEOUT_S) and no data is supplied through data_send before timeout (SERVER_SEND_THREAD_SLEEP_S)*/
void server_close();

#endif