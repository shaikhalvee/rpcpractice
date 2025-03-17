#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>

// For storing results keyed by rpcId
#include <errno.h>
#include <sys/types.h>

#define PORT 8081
#define BUF_SIZE 1024
#define MAX_RESULTS 1000

// We'll do a simple global array to store results
// In a real system, use a thread-safe map or dictionary
static char *results[MAX_RESULTS]; 
static int nextRpcId = 1000; 
pthread_mutex_t results_mutex = PTHREAD_MUTEX_INITIALIZER;

// Worker prototypes
long foo(long iterations);
int add_int(int i, int j);
void *compute_foo(void *arg);
void *compute_add(void *arg);
void *compute_sort(void *arg);

void handle_client(int client_sock);
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
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

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

    printf("[AsyncServer] Listening on port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }
        printf("[AsyncServer] Client connected.\n");

        // In a real system, you might do one thread per client
        // But for clarity, we'll handle them in this same thread
        handle_client(client_sock);
        close(client_sock);
        printf("[AsyncServer] Client disconnected.\n");
    }

    close(server_sock);
    return 0;
}

// parse request, handle immediate response
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

    // skip 'REQUEST:'
    char *cmd_str = buffer + 8;
    while (isspace(*cmd_str)) cmd_str++;

    char *method = strtok(cmd_str, " \t\r\n");
    if (!method) {
        char *errMsg = "RESPONSE: ERROR - No method\n";
        send(client_sock, errMsg, strlen(errMsg), 0);
        return;
    }

    if (strcmp(method, "foo") == 0) {
        // e.g., foo <iterations>
        char* iter_str = strtok(NULL, " \t\r\n");
        if (!iter_str) {
            char *errMsg = "RESPONSE: ERROR - Missing iterations\n";
            send(client_sock, errMsg, strlen(errMsg), 0);
            return;
        }
        long iterations = atol(iter_str);

        pthread_mutex_lock(&results_mutex);
        int rpcId = nextRpcId++;
        pthread_mutex_unlock(&results_mutex);

        // send immediate response
        char resp[64];
        snprintf(resp, sizeof(resp), "RESPONSE: %d\n", rpcId);
        send(client_sock, resp, strlen(resp), 0);

        // spin a thread to do the actual foo
        struct FooArg {
            int id;
            long iters;
        };
        struct FooArg *arg = malloc(sizeof(struct FooArg));
        arg->id = rpcId;
        arg->iters = iterations;

        pthread_t tid;
        pthread_create(&tid, NULL, compute_foo, arg);
        pthread_detach(tid);
    }
    else if (strcmp(method, "add") == 0) {
        // e.g., add <i> <j>
        char* i_str = strtok(NULL, " \t\r\n");
        char* j_str = strtok(NULL, " \t\r\n");
        if (!i_str || !j_str) {
            char *errMsg = "RESPONSE: ERROR - Missing operands\n";
            send(client_sock, errMsg, strlen(errMsg), 0);
            return;
        }
        int i = atoi(i_str);
        int j = atoi(j_str);

        pthread_mutex_lock(&results_mutex);
        int rpcId = nextRpcId++;
        pthread_mutex_unlock(&results_mutex);

        char resp[64];
        snprintf(resp, sizeof(resp), "RESPONSE: %d\n", rpcId);
        send(client_sock, resp, strlen(resp), 0);

        // create worker
        struct AddArg {
            int id;
            int a, b;
        };
        struct AddArg *arg = malloc(sizeof(struct AddArg));
        arg->id = rpcId;
        arg->a = i;
        arg->b = j;

        pthread_t tid;
        pthread_create(&tid, NULL, compute_add, arg);
        pthread_detach(tid);
    }
    else if (strcmp(method, "sort") == 0) {
        // e.g. sort <list_of_int>
        int vals[100];
        int count = 0;
        char *token;
        while ((token = strtok(NULL, " \t\r\n")) != NULL) {
            vals[count++] = atoi(token);
        }
        // store them in arg
        pthread_mutex_lock(&results_mutex);
        int rpcId = nextRpcId++;
        pthread_mutex_unlock(&results_mutex);

        char resp[64];
        snprintf(resp, sizeof(resp), "RESPONSE: %d\n", rpcId);
        send(client_sock, resp, strlen(resp), 0);

        struct SortArg {
            int id;
            int arr[100];
            int size;
        };
        struct SortArg *arg = malloc(sizeof(struct SortArg));
        arg->id = rpcId;
        arg->size = count;
        for (int i = 0; i < count; i++) {
            arg->arr[i] = vals[i];
        }

        pthread_t tid;
        pthread_create(&tid, NULL, compute_sort, arg);
        pthread_detach(tid);
    }
    else if (strcmp(method, "getResult") == 0) {
        // e.g. getResult <rpcId>
        char* id_str = strtok(NULL, " \t\r\n");
        if (!id_str) {
            char *errMsg = "RESPONSE: ERROR - Missing RPC id\n";
            send(client_sock, errMsg, strlen(errMsg), 0);
            return;
        }
        int queryId = atoi(id_str);

        pthread_mutex_lock(&results_mutex);
        if (queryId < 1000 || queryId >= 1000 + MAX_RESULTS) {
            pthread_mutex_unlock(&results_mutex);
            char *errMsg = "RESPONSE: ERROR - rpcId out of range\n";
            send(client_sock, errMsg, strlen(errMsg), 0);
            return;
        }
        char* res = results[queryId - 1000];
        pthread_mutex_unlock(&results_mutex);

        if (!res) {
            char *notReady = "RESPONSE: NOT_READY\n";
            send(client_sock, notReady, strlen(notReady), 0);
        } else {
            // return final result
            send(client_sock, "RESPONSE: ", 10, 0);
            send(client_sock, res, strlen(res), 0);
            send(client_sock, "\n", 1, 0);
        }
    }
    else {
        char *errMsg = "RESPONSE: ERROR - Unknown method\n";
        send(client_sock, errMsg, strlen(errMsg), 0);
    }
}

// store result in array
void store_result(int rpcId, const char *result) {
    pthread_mutex_lock(&results_mutex);
    if (rpcId - 1000 < MAX_RESULTS && rpcId - 1000 >= 0) {
        // free old result if any
        if (results[rpcId - 1000]) {
            free(results[rpcId - 1000]);
        }
        results[rpcId - 1000] = strdup(result);
    }
    pthread_mutex_unlock(&results_mutex);
}

// Worker threads
void *compute_foo(void *arg) {
    struct FooArg {
        int id;
        long iters;
    };
    struct FooArg *fa = (struct FooArg*)arg;

    long val = foo(fa->iters);
    // store result
    char buf[64];
    snprintf(buf, sizeof(buf), "OK: foo=%ld", val);
    store_result(fa->id, buf);

    free(fa);
    return NULL;
}

void *compute_add(void *arg) {
    struct AddArg {
        int id;
        int a, b;
    };
    struct AddArg *aa = (struct AddArg*)arg;

    int sum = add_int(aa->a, aa->b);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", sum);
    store_result(aa->id, buf);

    free(aa);
    return NULL;
}

void *compute_sort(void *arg) {
    struct SortArg {
        int id;
        int arr[100];
        int size;
    };
    struct SortArg *sa = (struct SortArg*)arg;
    
    // bubble sort
    for (int i = 0; i < sa->size - 1; i++) {
        for (int j = 0; j < sa->size - i - 1; j++) {
            if (sa->arr[j] > sa->arr[j+1]) {
                int tmp = sa->arr[j];
                sa->arr[j] = sa->arr[j+1];
                sa->arr[j+1] = tmp;
            }
        }
    }

    // build string e.g. "[1, 2, 3]"
    char buf[256];
    strcpy(buf, "[");
    for (int i = 0; i < sa->size; i++) {
        char tmp[16];
        snprintf(tmp, sizeof(tmp), "%d", sa->arr[i]);
        strcat(buf, tmp);
        if (i < sa->size - 1) strcat(buf, ", ");
    }
    strcat(buf, "]");
    store_result(sa->id, buf);

    free(sa);
    return NULL;
}

// dummy foo
long foo(long iterations) {
    long sum = 0;
    for (long i = 0; i < iterations; i++) {
        sum += i;
    }
    return sum;
}

int add_int(int i, int j) {
    return i + j;
}
