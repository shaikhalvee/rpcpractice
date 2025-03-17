#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 9090
#define BUFFER_SIZE 1024

int main() {
    int udp_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    socklen_t server_len = sizeof(server_addr);

    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    printf("[UDP Client] Enter messages:\n");
    while (1) {
        printf("Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        
        sendto(udp_sock, buffer, strlen(buffer), 0, 
               (struct sockaddr *)&server_addr, server_len);

        int bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0, 
                                      (struct sockaddr *)&server_addr, &server_len);
        if (bytes_received <= 0) {
            printf("[UDP Client] Server disconnected\n");
            break;
        }

        buffer[bytes_received] = '\0';
        printf("[UDP Client] Server echoed: %s\n", buffer);
    }

    close(udp_sock);
    return 0;
}
