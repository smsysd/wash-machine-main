#include "ipc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <stdio.h>

static void* _ipc_serverh(void* arg) {
	IpcServer* srv = (IpcServer*)arg;

	/* Основной цикл обработки подключений. */
    while (1) {
        /* Ожидание входящих подключений. */
        srv->data_sock = accept(srv->connection_sock, NULL, NULL);
		srv->onaccept(srv->data_sock);
        /* Закрытие сокета. */
        close(srv->data_sock);
		srv->data_sock = -1;
		if (srv->cancel) {
            break;
		}
    }
}

int ipc_server(IpcServer* srv, const char* path, int backlog, void (*onaccept)(int sock)) {
    int ret;
    int connection_socket;
	void* ptr;

	if (strlen(path) > 103 || onaccept == NULL) {
		return -1;
	}

    /*
     * Удалить сокет, оставшийся после последнего
     * некорректного завершения программы.
     */
    unlink(path);
    /* Создание локального сокета. */
    connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connection_socket == -1) {
        return -2;
    }
    /*
     * Для переносимости очищаем всю структуру, так как в некоторых
     * реализациях имеются дополнительные (нестандартные) поля.
     */
    memset(srv, 0, sizeof(IpcServer));
    /* Привязываем сокет к имени сокета. */
    srv->socka.sun_family = AF_UNIX;
    strncpy(srv->socka.sun_path, path, sizeof(srv->socka.sun_path) - 1);
    ret = bind(connection_socket, (const struct sockaddr*) &srv->socka, sizeof(struct sockaddr_un));
    if (ret == -1) {
        return -3;
    }
    /*
     * Готовимся принимать подключения. Размер очереди (backlog)
     * Пока один запрос обрабатывается, другие запросы смогут подождать.
     */
    ret = listen(connection_socket, backlog);
    if (ret == -1) {
    	return -4;
    }
	
	srv->connection_sock = connection_socket;
	srv->cancel = 0;
	srv->data_sock = -1;
	srv->onaccept = onaccept;
	
	/* Запуск потока обработки подключений */
	ret = pthread_create(&srv->thread, NULL, _ipc_serverh, srv);
	if (ret != 0) {
		return -5;
	}

    signal(SIGPIPE, SIG_IGN);
	return 0;
}

int ipc_server_destroy(IpcServer* srv) {
	/* Ожидание завершения обработки соединения */
	srv->cancel = 1;
	for (int i = 0; i < IPC_DESTROY_TIMEOUT_MS; i++) {
		// printf("%i\n", i);
        if (srv->data_sock == -1) {
			break;
		}
		usleep(1000);
	}

	/* Удаляем поток */
	pthread_cancel(srv->thread);
    
	/* Удаляем сокет */
    // printf("close\n");
	close(srv->connection_sock);
    // printf("unlink\n");
    unlink(srv->socka.sun_path);

	return 0;
}

int ipc_connect(const char* path) {
    struct sockaddr_un name;
    int ret;
    int data_socket;

    /* Создание локального сокета. */
    data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (data_socket == -1) {
        return -1;
    }
    /*
     * Для переносимости очищаем всю структуру, так как в некоторых
     * реализациях имеются дополнительные (нестандартные) поля.
     */
    memset(&name, 0, sizeof(struct sockaddr_un));
    /* Соединяем сокет с именем сокета. */
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, path, sizeof(name.sun_path) - 1);
    ret = connect(data_socket, (const struct sockaddr *) &name, sizeof(struct sockaddr_un));
    if (ret == -1) {
		return -2;
    }
	return data_socket;
}

int ipc_connect_close(int sock) {
	return close(sock);
}

int ipc_send(const char* path, const char* data, int nData) {
	int sock = ipc_connect(path);
	int ret;
	if (sock < 0) {
		return sock;
	}
	ret = write(sock, data, nData);
	if (ret == -1) {
		return -6;
	}
	return ipc_connect_close(sock);
}
