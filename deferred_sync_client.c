#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int send_request(const char *server_ip, int port, const char *req, char *resp);
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }
    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    // 1) Issue 'foo 1000000'
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);

    // Example: "REQUEST: foo 1000000"
    send_request(server_ip, port, "REQUEST: foo 1000000\n", buffer);
    // e.g. "RESPONSE: 1000"
    printf("[DeferredSyncClient] %s", buffer);
    int rpcId = -1;
    if (sscanf(buffer, "RESPONSE: %d", &rpcId) != 1) {
        printf("[DeferredSyncClient] Invalid response\n");
        return 1;
    }

    // 2) Now do 'waitResult <rpcId>'
    char waitCmd[64];
    snprintf(waitCmd, sizeof(waitCmd), "REQUEST: waitResult %d\n", rpcId);
    memset(buffer, 0, BUF_SIZE);
    send_request(server_ip, port, waitCmd, buffer);
    // e.g. "RESPONSE: OK: foo=xxxx"
    printf("[DeferredSyncClient] Final => %s", buffer);

    return 0;
}

int send_request(const char *server_ip, int port, const char *req, char *resp) {
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

    // read response
    ssize_t n = recv(sock, resp, BUF_SIZE - 1, 0);
    if (n < 0) {
        perror("recv");
    } else {
        resp[n] = '\0';
    }
    close(sock);
    return 0;
}
