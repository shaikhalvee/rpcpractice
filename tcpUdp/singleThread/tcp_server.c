#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>   // For sockaddr_in, inet_addr, etc.
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define PORT 8080
#define BUF_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUF_SIZE];

    // 1. Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // 2. Bind to port 8080
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind() failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 3. Listen for incoming connections
    if (listen(server_fd, 5) < 0) {
        perror("listen() failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("TCP server listening on port %d...\n", PORT);

    // 4. Accept client connection (single-threaded, so just do one client at a time)
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept() failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Accepted a client!\n");

    // 5. Receive request
    memset(buffer, 0, BUF_SIZE);
    ssize_t recv_len = recv(client_fd, buffer, BUF_SIZE, 0);
    if (recv_len < 0) {
        perror("recv() failed");
        close(client_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Client says: %s\n", buffer);

    // Here, parse which image the client wants. For example:
    // Suppose the client says "GET image1.jpg"

    // 6. "Rename" the image as needed. For instance:
    // char renamedFilename[256];
    // sprintf(renamedFilename, "%s_%d", requestedFilename, client_fd);

    // 7. Open the image file, read it into a buffer, send it back
    // In a real server you'd loop over file contents, but here's a placeholder:

    char *response = "Here is your (fake) image data...";
    send(client_fd, response, strlen(response), 0);

    // 8. Close the connection
    close(client_fd);
    close(server_fd);
    return 0;
}
