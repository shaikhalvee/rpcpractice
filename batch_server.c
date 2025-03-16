#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>

#define PORT 8083
#define BUF_SIZE 1024
#define MAX_LINES 10

long foo(long iterations);
int add_int(int i, int j);
void sort_array(int* arr, int size);

// We'll let the client send something like:
// REQUEST: batch
// add 3 5
// foo 10000
// sort 9 7 1
// END

void handle_client(int client_sock);

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

    printf("[BatchServer] Listening on port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }
        printf("[BatchServer] Client connected.\n");

        handle_client(client_sock);

        close(client_sock);
        printf("[BatchServer] Client disconnected.\n");
    }

    close(server_sock);
    return 0;
}

void handle_client(int client_sock) {
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);

    // read the first line
    ssize_t n = recv(client_sock, buffer, BUF_SIZE - 1, 0);
    if (n <= 0) {
        perror("recv");
        return;
    }
    buffer[n] = '\0';

    // e.g. "REQUEST: batch\n"
    if (strncmp(buffer, "REQUEST: batch", 14) != 0) {
        char *errMsg = "RESPONSE: ERROR - Not a batch request\n";
        send(client_sock, errMsg, strlen(errMsg), 0);
        return;
    }

    // Now read lines until we see 'END'
    char result[BUF_SIZE * 2];
    memset(result, 0, sizeof(result));
    strcat(result, "RESPONSE:\n");

    while (1) {
        memset(buffer, 0, BUF_SIZE);
        n = recv(client_sock, buffer, BUF_SIZE - 1, 0);
        if (n <= 0) {
            perror("recv lines");
            break;
        }
        buffer[n] = '\0';

        // if user typed 'END', we stop
        if (strncmp(buffer, "END", 3) == 0) {
            break;
        }

        // parse line e.g. "add 3 5", "foo 10000", "sort 9 7 1"
        char *method = strtok(buffer, " \t\r\n");
        if (!method) {
            // skip
            continue;
        }
        if (strcmp(method, "add") == 0) {
            char* i_str = strtok(NULL, " \t\r\n");
            char* j_str = strtok(NULL, " \t\r\n");
            if (!i_str || !j_str) {
                strcat(result, "ERROR(add): missing operands\n");
            } else {
                int i = atoi(i_str);
                int j = atoi(j_str);
                int s = add_int(i, j);
                char tmp[32];
                snprintf(tmp, sizeof(tmp), "add(%d,%d)=%d\n", i, j, s);
                strcat(result, tmp);
            }
        }
        else if (strcmp(method, "foo") == 0) {
            char* iter_str = strtok(NULL, " \t\r\n");
            if (!iter_str) {
                strcat(result, "ERROR(foo): missing iterations\n");
            } else {
                long iters = atol(iter_str);
                long sum = foo(iters);
                char tmp[64];
                snprintf(tmp, sizeof(tmp), "foo(%ld)=%ld\n", iters, sum);
                strcat(result, tmp);
            }
        }
        else if (strcmp(method, "sort") == 0) {
            int arr[100];
            int count = 0;
            char* token;
            while ((token = strtok(NULL, " \t\r\n")) != NULL) {
                arr[count++] = atoi(token);
            }
            sort_array(arr, count);
            strcat(result, "sort => [");
            for (int k = 0; k < count; k++) {
                char tmp[16];
                snprintf(tmp, sizeof(tmp), "%d", arr[k]);
                strcat(result, tmp);
                if (k < count - 1) strcat(result, ",");
            }
            strcat(result, "]\n");
        }
        else {
            strcat(result, "ERROR: Unknown command\n");
        }
    }

    // send entire batch result
    send(client_sock, result, strlen(result), 0);
}

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

void sort_array(int* arr, int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - 1 - i; j++) {
            if (arr[j] > arr[j+1]) {
                int tmp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = tmp;
            }
        }
    }
}
