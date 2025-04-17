#ifndef THREADS_H
#define THREADS_H

#include <stdbool.h>
#include <netinet/in.h>

typedef struct {
    int client_fd;
    bool verbose;
    struct sockaddr_in client_addr;
} client_conn_args_t;

int create_client_handler_thread(client_conn_args_t *args);

#endif