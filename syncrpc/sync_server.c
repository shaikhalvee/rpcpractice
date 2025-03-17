#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF_SIZE 1024

// Prototypes
void handle_client(int client_sock);
long foo(long iterations);
int add_int(int i, int j);
void sort_array(int* arr, int size);

// Main server
int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // 1) Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 2) Bind
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 3) Listen
    if (listen(server_sock, 5) < 0) {
        perror("listen");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("[SyncServer] Listening on port %d...\n", PORT);

    // 4) Accept clients in a loop
    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("accept");
            continue; 
        }
        printf("[SyncServer] Client connected.\n");

        // Handle client in a blocking fashion
        handle_client(client_sock);
        close(client_sock);
        printf("[SyncServer] Client disconnected.\n");
    }

    close(server_sock);
    return 0;
}

// Handle a single client request (blocking / synchronous)
void handle_client(int client_sock) {
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);

    // 1) Read request from client
    ssize_t n = recv(client_sock, buffer, BUF_SIZE - 1, 0);
    if (n <= 0) {
        perror("recv");
        return;
    }
    buffer[n] = '\0';

    // Example request format:
    //   "REQUEST: foo 100000\n"
    //   "REQUEST: add 3 5\n"
    //   "REQUEST: sort 5 9 1 3 2\n"
    if (strncmp(buffer, "REQUEST:", 8) != 0) {
        char *errMsg = "RESPONSE: ERROR - Invalid request\n";
        send(client_sock, errMsg, strlen(errMsg), 0);
        return;
    }

    // Parse the command
    char* cmd_str = buffer + 8; // skip "REQUEST:"
    while (*cmd_str == ' ') cmd_str++; // skip leading spaces

    // We'll tokenize it
    // e.g., "foo 100000"
    char *method = strtok(cmd_str, " \t\r\n");
    if (!method) {
        char *errMsg = "RESPONSE: ERROR - No method\n";
        send(client_sock, errMsg, strlen(errMsg), 0);
        return;
    }

    char response[BUF_SIZE];
    memset(response, 0, BUF_SIZE);

    if (strcmp(method, "foo") == 0) {
        // e.g., foo 100000
        char* iter_str = strtok(NULL, " \t\r\n");
        if (!iter_str) {
            snprintf(response, BUF_SIZE, "RESPONSE: ERROR - Missing iterations\n");
        } else {
            long iterations = atol(iter_str);
            foo(iterations); 
            // We won't return the sum for synchronous example; we just do dummy work
            snprintf(response, BUF_SIZE, "RESPONSE: OK\n");
        }
    }
    else if (strcmp(method, "add") == 0) {
        // e.g., add 3 5
        char* i_str = strtok(NULL, " \t\r\n");
        char* j_str = strtok(NULL, " \t\r\n");
        if (!i_str || !j_str) {
            snprintf(response, BUF_SIZE, "RESPONSE: ERROR - Missing operands\n");
        } else {
            int i = atoi(i_str);
            int j = atoi(j_str);
            int sum = add_int(i, j);
            snprintf(response, BUF_SIZE, "RESPONSE: %d\n", sum);
        }
    }
    else if (strcmp(method, "sort") == 0) {
        // e.g., sort 5 9 1 3 2
        int vals[100];
        int count = 0;

        char *token;
        while ((token = strtok(NULL, " \t\r\n")) != NULL) {
            vals[count++] = atoi(token);
        }
        sort_array(vals, count);

        // build the response e.g. "[1, 2, 3, 5, 9]"
        char tmp[16];
        strcat(response, "RESPONSE: [");
        for (int x = 0; x < count; x++) {
            snprintf(tmp, sizeof(tmp), "%d", vals[x]);
            strcat(response, tmp);
            if (x < count - 1) strcat(response, ", ");
        }
        strcat(response, "]\n");
    }
    else {
        snprintf(response, BUF_SIZE, "RESPONSE: ERROR - Unknown method\n");
    }

    // 2) Send response
    send(client_sock, response, strlen(response), 0);
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

void sort_array(int* arr, int size) {
    // simple bubble sort for demo
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (arr[j] > arr[j+1]) {
                int tmp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = tmp;
            }
        }
    }
}
