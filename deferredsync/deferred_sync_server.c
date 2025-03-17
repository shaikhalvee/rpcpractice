#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>

#define PORT 8082
#define BUF_SIZE 1024
#define MAX_RESULTS 1000

static char *results[MAX_RESULTS];
static int nextRpcId = 1000;
pthread_mutex_t results_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int rpcId;
    long iterations;
} FooArg;

// Prototypes
void handle_client(int client_sock);
void *compute_foo(void *arg);
void store_result(int rpcId, const char *result);

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) < 0) {
        perror("listen");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("[DeferredSyncServer] Listening on port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }
        printf("[DeferredSyncServer] Client connected.\n");

        // handle client (one request at a time for simplicity)
        handle_client(client_sock);

        close(client_sock);
        printf("[DeferredSyncServer] Client disconnected.\n");
    }

    close(server_sock);
    return 0;
}

void handle_client(int client_sock) {
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);

    ssize_t n = recv(client_sock, buffer, BUF_SIZE - 1, 0);
    if (n <= 0) {
        perror("recv");
        return;
    }
    buffer[n] = '\0';

    if (strncmp(buffer, "REQUEST:", 8) != 0) {
        char *errMsg = "RESPONSE: ERROR - Invalid request\n";
        send(client_sock, errMsg, strlen(errMsg), 0);
        return;
    }

    char *cmd_str = buffer + 8;
    while (isspace((unsigned char)*cmd_str)) cmd_str++;

    char *method = strtok(cmd_str, " \t\r\n");
    if (!method) {
        char *errMsg = "RESPONSE: ERROR - No method\n";
        send(client_sock, errMsg, strlen(errMsg), 0);
        return;
    }

    if (strcmp(method, "foo") == 0) {
        // 'foo <iterations>'
        char* iter_str = strtok(NULL, " \t\r\n");
        if (!iter_str) {
            char *errMsg = "RESPONSE: ERROR - Missing iterations\n";
            send(client_sock, errMsg, strlen(errMsg), 0);
            return;
        }
        long iters = atol(iter_str);

        pthread_mutex_lock(&results_mutex);
        int rpcId = nextRpcId++;
        pthread_mutex_unlock(&results_mutex);

        // respond with the rpcId
        char resp[64];
        snprintf(resp, sizeof(resp), "RESPONSE: %d\n", rpcId);
        send(client_sock, resp, strlen(resp), 0);

        // spawn worker
        FooArg *fa = malloc(sizeof(FooArg));
        fa->rpcId = rpcId;
        fa->iterations = iters;
        pthread_t tid;
        pthread_create(&tid, NULL, compute_foo, fa);
        pthread_detach(tid);
    }
    else if (strcmp(method, "waitResult") == 0) {
        // 'waitResult <rpcId>'
        char* id_str = strtok(NULL, " \t\r\n");
        if (!id_str) {
            char *errMsg = "RESPONSE: ERROR - Missing RPC ID\n";
            send(client_sock, errMsg, strlen(errMsg), 0);
            return;
        }
        int queryId = atoi(id_str);
        if (queryId < 1000 || queryId >= 1000 + MAX_RESULTS) {
            char *errMsg = "RESPONSE: ERROR - rpcId out of range\n";
            send(client_sock, errMsg, strlen(errMsg), 0);
            return;
        }
        // We'll block until the result is ready
        // For a minimal demo, we do naive polling
        while (1) {
            pthread_mutex_lock(&results_mutex);
            char *res = results[queryId - 1000];
            pthread_mutex_unlock(&results_mutex);
            if (res) {
                // send final result
                send(client_sock, "RESPONSE: ", 10, 0);
                send(client_sock, res, strlen(res), 0);
                send(client_sock, "\n", 1, 0);
                return;
            }
            // Sleep briefly to avoid busy loop
            usleep(100000); 
        }
    }
    else {
        char *errMsg = "RESPONSE: ERROR - Unknown method\n";
        send(client_sock, errMsg, strlen(errMsg), 0);
    }
}

// Worker that does foo(...) in background
void *compute_foo(void *arg) {
    FooArg *fa = (FooArg*)arg;
    long iterations = fa->iterations;
    int rpcId = fa->rpcId;

    long sum = 0;
    for (long i = 0; i < iterations; i++) {
        sum += i;
    }

    char buf[64];
    snprintf(buf, sizeof(buf), "OK: foo=%ld", sum);
    store_result(rpcId, buf);

    free(fa);
    return NULL;
}

void store_result(int rpcId, const char *result) {
    pthread_mutex_lock(&results_mutex);
    if (rpcId - 1000 >= 0 && rpcId - 1000 < MAX_RESULTS) {
        if (results[rpcId - 1000]) {
            free(results[rpcId - 1000]);
        }
        results[rpcId - 1000] = strdup(result);
    }
    pthread_mutex_unlock(&results_mutex);
}
