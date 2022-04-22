#ifndef IPC
#define IPC

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#define IPC_DESTROY_TIMEOUT_MS		2000

typedef struct IpcServer {
	pthread_t thread;
	struct sockaddr_un socka;
	int id;
	int connection_sock;
	int data_sock;
	char cancel;
	void (*onaccept)(int);
} IpcServer;

/* * * Create new thread for server process * * *
* return 0, if success - id, else error
* * */
int ipc_server(IpcServer* srv, const char* path, int backlog, void (*onaccept)(int sock));

int ipc_server_destroy(IpcServer* srv);

/* * * Create connection
* return value >= 0, if success - socket_id, else error
* * */
int ipc_connect(const char* path);

int ipc_connect_close(int sock);

/* * * Create connection, write data to socket, destroy connection
* return 0, if success
* * */
int ipc_send(const char* path, const char* data, int nData);

#ifdef __cplusplus
}
#endif

#endif