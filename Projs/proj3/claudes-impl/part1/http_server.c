#define _GNU_SOURCE

#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "connection_queue.h"
#include "http.h"

#define N_THREADS 5
#define BUFSIZE 512    // As in http.c: read_http_request() needs a buffer this size
#define LISTEN_QUEUE_LEN 5

volatile sig_atomic_t keep_going = 1;
const char *serve_dir;

void handle_sigint(int signo) { keep_going = 0; }

// getaddrinfo(), socket(), bind(), listen(). Returns the server socket or -1.
static int setup_server_socket(const char *port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;    // A server: bind to every local address
    struct addrinfo *server;
    int ret_val = getaddrinfo(NULL, port, &hints, &server);
    if (ret_val != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret_val));
        return -1;
    }

    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (sock_fd == -1) {
        perror("socket");
        freeaddrinfo(server);
        return -1;
    }
    if (bind(sock_fd, server->ai_addr, server->ai_addrlen) == -1) {
        perror("bind");
        freeaddrinfo(server);
        close(sock_fd);
        return -1;
    }
    freeaddrinfo(server);
    if (listen(sock_fd, LISTEN_QUEUE_LEN) == -1) {
        perror("listen");
        close(sock_fd);
        return -1;
    }
    return sock_fd;
}

// Reads one request and answers it. Errors are reported where they happen;
// either way the caller closes the connection.
static void serve_client(int client_fd) {
    char resource_name[BUFSIZE];
    if (read_http_request(client_fd, resource_name) == -1) {
        return;
    }
    // "/quote.txt" in "server_files" -> "server_files/quote.txt"
    char resource_path[PATH_MAX];
    int len = snprintf(resource_path, sizeof(resource_path), "%s%s", serve_dir, resource_name);
    if (len < 0 || (size_t) len >= sizeof(resource_path)) {
        fprintf(stderr, "serve_client: resource path too long\n");
        return;
    }
    write_http_response(client_fd, resource_path);
}

static void *worker_thread(void *arg) {
    connection_queue_t *queue = arg;
    int client_fd;
    // -1 once the queue is shut down and empty (or a pthread call failed)
    while ((client_fd = connection_queue_dequeue(queue)) != -1) {
        serve_client(client_fd);
        if (close(client_fd) == -1) {
            perror("close");
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    // First argument is directory to serve, second is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }
    serve_dir = argv[1];
    const char *port = argv[2];

    // No SA_RESTART: SIGINT must make a blocked accept() fail with EINTR
    struct sigaction sact;
    sact.sa_handler = handle_sigint;
    sact.sa_flags = 0;
    if (sigfillset(&sact.sa_mask) == -1) {
        perror("sigfillset");
        return 1;
    }
    if (sigaction(SIGINT, &sact, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    int sock_fd = setup_server_socket(port);
    if (sock_fd == -1) {
        return 1;
    }

    connection_queue_t queue;
    if (connection_queue_init(&queue) == -1) {
        close(sock_fd);
        return 1;
    }

    // Threads inherit their creator's signal mask. Block every signal while
    // creating the workers, so the kernel can only deliver SIGINT to main.
    int exit_code = 0;
    sigset_t all_signals;
    sigset_t old_mask;
    if (sigfillset(&all_signals) == -1) {
        perror("sigfillset");
        connection_queue_free(&queue);
        close(sock_fd);
        return 1;
    }
    if (sigprocmask(SIG_BLOCK, &all_signals, &old_mask) == -1) {
        perror("sigprocmask");
        connection_queue_free(&queue);
        close(sock_fd);
        return 1;
    }
    pthread_t workers[N_THREADS];
    int n_workers = 0;
    while (n_workers < N_THREADS) {
        int result = pthread_create(&workers[n_workers], NULL, worker_thread, &queue);
        if (result != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(result));
            exit_code = 1;
            break;
        }
        n_workers++;
    }
    if (sigprocmask(SIG_SETMASK, &old_mask, NULL) == -1) {
        perror("sigprocmask");
        exit_code = 1;    // SIGINT would stay blocked: shut down now instead
    }

    // Main thread only accepts; a worker talks to the client
    while (keep_going && exit_code == 0) {
        int client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd == -1) {
            if (errno != EINTR) {
                perror("accept");
                exit_code = 1;
            }
            continue;    // EINTR: the loop condition sees keep_going == 0
        }
        // Blocks while the queue is full, capping the number of waiting clients
        if (connection_queue_enqueue(&queue, client_fd) == -1) {
            if (close(client_fd) == -1) {
                perror("close");
            }
            exit_code = 1;
        }
    }

    // Stop accepting first, so a client that connects during shutdown is
    // refused rather than left waiting in the backlog until the process exits
    if (close(sock_fd) == -1) {
        perror("close");
        exit_code = 1;
    }
    // Workers finish their current client, drain the queue, then return
    if (connection_queue_shutdown(&queue) == -1) {
        exit_code = 1;
    }
    for (int i = 0; i < n_workers; i++) {
        int result = pthread_join(workers[i], NULL);
        if (result != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(result));
            exit_code = 1;
        }
    }
    if (connection_queue_free(&queue) == -1) {
        exit_code = 1;
    }
    return exit_code;
}
