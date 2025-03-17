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
    socklen_t serv_addr_len = sizeof(serv_addr);
    char buffer[BUF_SIZE];

    // 1. Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // 2. Prepare server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("inet_pton() failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 3. Send a request (e.g., "GET image1.jpg")
    char *request = "GET image1.jpg";
    if (sendto(sockfd, request, strlen(request), 0, (struct sockaddr *)&serv_addr, serv_addr_len) < 0) {
        perror("sendto() failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 4. Receive server’s response
    memset(buffer, 0, BUF_SIZE);
    ssize_t recv_len = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&serv_addr, &serv_addr_len);
    if (recv_len > 0) {
        printf("Server replied: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}
