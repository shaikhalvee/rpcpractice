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
    struct sockaddr_in serv_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUF_SIZE];

    // 1. Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // 2. Bind to port
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind() failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("UDP server listening on port %d...\n", PORT);

    // 3. Receive request from client
    memset(buffer, 0, BUF_SIZE);
    ssize_t recv_len = recvfrom(sockfd, buffer, BUF_SIZE, 0,
                                (struct sockaddr *)&client_addr, &client_addr_len);
    if (recv_len < 0) {
        perror("recvfrom() failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Received from client: %s\n", buffer);

    // 4. "Rename" the requested image, read it, etc. Then respond
    char *response = "Here is your UDP-based (fake) image data...";
    if (sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)&client_addr, client_addr_len) < 0) {
        perror("sendto() failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    close(sockfd);
    return 0;
}
