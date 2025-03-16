#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <server_ip> <port> <method_and_args>\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 8080 \"REQUEST: add 3 5\"\n", argv[0]);
        return 1;
    }
    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    char *request_str = argv[3]; // we expect something like "REQUEST: foo 100000"

    // 1) Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // 2) Connect
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

    // 3) Send request
    send(sock, request_str, strlen(request_str), 0);
    // Also send a newline or similar, so server can parse easily
    send(sock, "\n", 1, 0);

    // 4) Receive response
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);
    ssize_t n = recv(sock, buffer, BUF_SIZE - 1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        printf("[SyncClient] Server response: %s", buffer);
    }

    close(sock);
    return 0;
}
