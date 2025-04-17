#define _GNU_SOURCE // For getopt, etc.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#include "threads.h"

#define DEFAULT_PORT 8080

int main(int argc, char *argv[]) {
    int opt;
    int server_port = DEFAULT_PORT;
    bool verbose_mode = false;
    int listen_socket_fd = -1;
    struct sockaddr_in server_addr;
    int reuse_addr = 1;

    // Handle arguments
    while ((opt = getopt(argc, argv, "p:v")) != -1) {
        switch (opt) {
            case 'p':
                server_port = atoi(optarg);
                if (server_port <= 0 || server_port > 65535) {
                    fprintf(stderr, "Invalid port: %s. Using default %d.\n",
                            optarg, DEFAULT_PORT);
                    server_port = DEFAULT_PORT;
                }
                break;
            case 'v':
                verbose_mode = true;
                break; 
            // Handle invalid options or missing args for -p
            case '?':                
                fprintf(stderr, "Usage: %s [-p port] [-v]\n", argv[0]);
                exit(EXIT_FAILURE);
            default:
                abort();
        }
    }

    printf("Starting server on port %d...\n", server_port);
    if (verbose_mode) {
        printf("Verbose mode enabled.\n");
    }

    // Create TCP Socket
    listen_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket_fd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Allow reuse of address/port
    if (setsockopt(listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    // Setup server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on any interface
    server_addr.sin_port = htons(server_port); // Port in network byte order

    // Bind socket to address and port
    if (bind(listen_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(listen_socket_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(listen_socket_fd, 10) < 0) { // Backlog of 10 connections
        perror("listen failed");
        close(listen_socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d. Waiting for connections...\n", server_port);

    // Main loop for connection accepted
    while (true) {
        // Allocate args struct on heap for the new thread
        client_conn_args_t *args = malloc(sizeof(client_conn_args_t));
        if (!args) {
            perror("malloc thread args failed");
            continue;
        }
        args->verbose = verbose_mode;

        socklen_t client_addr_len = sizeof(args->client_addr);

        args->client_fd = accept(listen_socket_fd,
                                 (struct sockaddr *)&(args->client_addr),
                                 &client_addr_len);

        if (args->client_fd < 0) {
            // Failed to accept
            perror("accept failed");
            free(args);
            continue;
        }

        // Log the accepted connection
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(args->client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Accepted connection from %s:%d (FD: %d)\n",
               client_ip, ntohs(args->client_addr.sin_port), args->client_fd);

        // Call function in threads.c to create and detach the handler thread
        if (create_client_handler_thread(args) != 0) {
            // Thread creation failed: clean up socket and args here
            fprintf(stderr, "Failed to create handler thread for %s:%d\n", client_ip, ntohs(args->client_addr.sin_port));
            close(args->client_fd);
            free(args);
        }
    }

    printf("Server shutting down.\n");
    close(listen_socket_fd);

    return 0;
}