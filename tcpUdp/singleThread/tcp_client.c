#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUF_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE];

    // 1. Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // 2. Specify server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // If server is on the same machine, use 127.0.0.1
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("inet_pton() failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 3. Connect to server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect() failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 4. Send a “fake” request for an image
    char *request = "GET image1.jpg";
    send(sockfd, request, strlen(request), 0);

    // 5. Receive the image data (in this example, just a text string)
    memset(buffer, 0, BUF_SIZE);
    ssize_t recv_len = recv(sockfd, buffer, BUF_SIZE, 0);
    if (recv_len > 0) {
        printf("Received from server: %s\n", buffer);
    }

    // 6. Close the socket
    close(sockfd);
    return 0;
}
