#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024
#define ROOT_DIR "www"  // The directory where files are stored

void send_response(SOCKET client_socket, int status_code, const char *content_type, const char *file_path);

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    // Configure server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Bind failed\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Start listening
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        printf("Listen failed\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    printf("Server is running on port %d\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) == INVALID_SOCKET) {
            printf("Accept failed\n");
            closesocket(server_fd);
            WSACleanup();
            return 1;
        }

        // Read client request
        recv(new_socket, buffer, BUFFER_SIZE, 0);
        printf("Received request:\n%s\n", buffer);

        // Extract the requested file from the HTTP GET request
        char method[10], file_path[100];
        sscanf(buffer, "%s %s", method, file_path);

        // Only handle GET requests
        if (strcmp(method, "GET") != 0) {
            send_response(new_socket, 405, "text/plain", NULL);
        } else {
            // Default to index.html if root is requested
            if (strcmp(file_path, "/") == 0) {
                strcpy(file_path, "/index.html");
            }

            // Construct the full file path
            char full_path[150];
            snprintf(full_path, sizeof(full_path), "%s%s", ROOT_DIR, file_path);

            // Send file response
            send_response(new_socket, 200, "text/html", full_path);
        }

        closesocket(new_socket);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}

// Function to send an HTTP response with a file
void send_response(SOCKET client_socket, int status_code, const char *content_type, const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    char response[BUFFER_SIZE];
    
    if (!file) {
        // If file is not found, return a 404 error
        const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
                                "<html><body><h1>404 Not Found</h1></body></html>";
        send(client_socket, not_found, strlen(not_found), 0);
    } else {
        // Send HTTP response headers
        snprintf(response, sizeof(response), "HTTP/1.1 %d OK\r\nContent-Type: %s\r\n\r\n", status_code, content_type);
        send(client_socket, response, strlen(response), 0);

        // Read and send the file in chunks
        char file_buffer[BUFFER_SIZE];
        int bytes_read;
        while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, file)) > 0) {
            send(client_socket, file_buffer, bytes_read, 0);
        }
        fclose(file);
    }
}
