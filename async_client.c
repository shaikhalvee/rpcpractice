#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }
    char *server_ip = argv[1];
    int port = atoi(argv[2]);

    // Example usage: we'll do an async foo, async add, async sort
    // Then poll for results
    int fooId = sendRequest(server_ip, port, "REQUEST: foo 1000000");
    printf("[AsyncClient] foo(1000000) => rpcId=%d\n", fooId);

    int addId = sendRequest(server_ip, port, "REQUEST: add 3 5");
    printf("[AsyncClient] add(3,5) => rpcId=%d\n", addId);

    int sortId = sendRequest(server_ip, port, "REQUEST: sort 9 5 1 2 8");
    printf("[AsyncClient] sort(9,5,1,2,8) => rpcId=%d\n", sortId);

    // Now poll for each
    for (int i = 0; i < 10; i++) {
        sleep(1);
        char getFoo[64];
        snprintf(getFoo, sizeof(getFoo), "REQUEST: getResult %d", fooId);
        int rc = sendGetResult(server_ip, port, getFoo);
        if (rc == 0) break; // means we got final result

        // same for addId or sortId if we want
    }

    return 0;
}

// This function sends a request like "REQUEST: foo 100000" and expects "RESPONSE: <rpcId>"
// returns <rpcId> as integer
int sendRequest(const char *server_ip, int port, const char *req) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    // send request
    send(sock, req, strlen(req), 0);
    send(sock, "\n", 1, 0);

    // read response
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);
    ssize_t n = recv(sock, buffer, BUF_SIZE - 1, 0);
    close(sock);

    if (n <= 0) {
        return -1;
    }
    buffer[n] = '\0';

    // e.g. "RESPONSE: 1001"
    if (strncmp(buffer, "RESPONSE:", 9) != 0) {
        printf("[AsyncClient] Invalid response: %s\n", buffer);
        return -1;
    }
    char* val_str = buffer + 9; // skip 'RESPONSE:'
    while (*val_str == ' ') val_str++;
    return atoi(val_str);
}

// This function sends a request like "REQUEST: getResult 1001"
// If we get "RESPONSE: NOT_READY", we return 1
// Otherwise, we print the final result and return 0
int sendGetResult(const char *server_ip, int port, const char *req) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    // send request
    send(sock, req, strlen(req), 0);
    send(sock, "\n", 1, 0);

    // read response
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);
    ssize_t n = recv(sock, buffer, BUF_SIZE - 1, 0);
    close(sock);

    if (n <= 0) return 1;
    buffer[n] = '\0';

    if (strncmp(buffer, "RESPONSE:", 9) != 0) {
        printf("[AsyncClient] Invalid response: %s\n", buffer);
        return 1;
    }
    char* val_str = buffer + 9;
    while (*val_str == ' ') val_str++;

    if (strncmp(val_str, "NOT_READY", 9) == 0) {
        printf("[AsyncClient] Not ready yet\n");
        return 1;
    } else {
        printf("[AsyncClient] Result = %s\n", val_str);
        return 0; 
    }
}
