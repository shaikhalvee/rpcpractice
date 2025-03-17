#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 9090
#define BUFFER_SIZE 1024

void *handle_request(void *arg) {
    int sock = *((int *)arg);
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    
    while (1) {
        int bytes_received = recvfrom(sock, buffer, BUFFER_SIZE, 0,
                                      (struct sockaddr *)&client_addr, &client_len);
        if (bytes_received < 0) {
            perror("Receive failed");
            continue;
        }

        buffer[bytes_received] = '\0';
        printf("[UDP Server] Received: %s\n", buffer);
        
        sendto(sock, buffer, bytes_received, 0, 
               (struct sockaddr *)&client_addr, client_len);
    }
    
    return NULL;
}

int main() {
    int udp_sock;
    struct sockaddr_in server_addr;
    pthread_t thread_id;

    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(udp_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("[UDP Server] Listening on port %d...\n", PORT);
    pthread_create(&thread_id, NULL, handle_request, &udp_sock);
    pthread_join(thread_id, NULL);

    close(udp_sock);
    return 0;
}
