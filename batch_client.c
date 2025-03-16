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

    // We'll send a batch with add, foo, sort
    // Then 'END'
    char batchRequest[] =
        "REQUEST: batch\n"
        "add 3 5\n"
        "foo 1000\n"
        "sort 9 5 1 2 8\n"
        "END\n";

    send(sock, batchRequest, strlen(batchRequest), 0);

    // read server response
    char buffer[BUF_SIZE * 2];
    memset(buffer, 0, sizeof(buffer));
    ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        printf("[BatchClient] Server Response:\n%s", buffer);
    }

    close(sock);
    return 0;
}
