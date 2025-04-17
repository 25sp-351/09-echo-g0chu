#include "threads.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

// Echoes data back
static void *client_handler_thread_func(void *arg) {
    client_conn_args_t *args = (client_conn_args_t *)arg;
    int client_fd = args->client_fd;
    bool verbose = args->verbose;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(args->client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(args->client_addr.sin_port);

    char buffer[BUFFER_SIZE];
    FILE *read_stream = NULL;
    FILE *write_stream = NULL;
    int read_fd = -1;

    // Dup fd for read stream
    read_fd = dup(client_fd);
    if (read_fd < 0) {
        perror("dup read failed");
        goto cleanup;
    }
    read_stream = fdopen(read_fd, "r");
    if (!read_stream) {
        perror("fdopen read failed");
        goto cleanup;
    }
    // Use original fd for write stream
    write_stream = fdopen(client_fd, "w");
    if (!write_stream) {
        perror("fdopen write failed");
        goto cleanup;
    }
    setvbuf(write_stream, NULL, _IOLBF, 0);

    // Echo loop
    while (fgets(buffer, sizeof(buffer), read_stream) != NULL) {
        if (verbose) {
            printf("[Thr %lu, %s:%d] Rcvd: %s", (unsigned long)pthread_self(), client_ip, client_port, buffer);
            fflush(stdout);
        }
        // Echo back
        if (fprintf(write_stream, "%s", buffer) < 0 || fflush(write_stream) != 0) {
             fprintf(stderr, "[Thr %lu, %s:%d] Write/flush error or client disconnected.\n", (unsigned long)pthread_self(), client_ip, client_port);
             break;
        }
    }

    // Check if loop exited due to read error or clean disconnect
    if (ferror(read_stream)) {
        perror("read error from client");
    } else {
        printf("[Thr %lu, %s:%d] Client disconnected.\n", (unsigned long)pthread_self(), client_ip, client_port);
    }

cleanup:
    // Cleanup resources
    if (read_stream) fclose(read_stream);
    else if (read_fd >= 0) close(read_fd);

    if (write_stream) fclose(write_stream);
    else if (client_fd >= 0 && read_stream) {
         close(client_fd);
     } else if (client_fd >= 0 && !read_stream && read_fd < 0) {
          close(client_fd);
     }


    printf("[Thr %lu, %s:%d] Handler thread exiting.\n", (unsigned long)pthread_self(), client_ip, client_port);
    free(args);
    return NULL;
}

int create_client_handler_thread(client_conn_args_t *args) {
    pthread_t thread_id;
    int rc = pthread_create(&thread_id, NULL, client_handler_thread_func, args);
    if (rc != 0) {
        perror("pthread_create failed");
        return rc;
    }

    pthread_detach(thread_id);

    return 0;
}