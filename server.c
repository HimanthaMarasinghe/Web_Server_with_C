#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>  // For access() function
#include <windows.h>  // For multithreading

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to determine content type based on file extension
const char *get_content_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return NULL;  // No extension found, treat as unsupported

    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".mp4") == 0) return "video/mp4";
    if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";

    return NULL;  // Unsupported media type
}

// Function to send response
void send_response(SOCKET client_socket, const char *file_path, const char *method) {
    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "./root%s", file_path);

    // Only allow GET requests
    if (strcmp(method, "GET") != 0) {
        const char *method_not_allowed = "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/html\r\n\r\n"
                                         "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        send(client_socket, method_not_allowed, strlen(method_not_allowed), 0);
        return;
    }

    // Prevent directory traversal attacks
    if (strstr(file_path, "../")) {  // 4 checks for read permission (R_OK)
        const char *forbidden = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\n\r\n"
                                "<html><body><h1>403 Forbidden</h1></body></html>";
        send(client_socket, forbidden, strlen(forbidden), 0);
        return;
    }

    // Check file existence and permissions
    if (_access_s(full_path, 0) != 0) {  // 0 checks for existence (F_OK)
        const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
                                "<html><body><h1>404 Not Found</h1></body></html>";
        send(client_socket, not_found, strlen(not_found), 0);
        return;
    }

    // Get content type
    const char *content_type = get_content_type(file_path);
    if (!content_type) {
        const char *unsupported_media = "HTTP/1.1 415 Unsupported Media Type\r\nContent-Type: text/html\r\n\r\n"
                                        "<html><body><h1>415 Unsupported Media Type</h1></body></html>";
        send(client_socket, unsupported_media, strlen(unsupported_media), 0);
        return;
    }

    // Open and send file
    FILE *file = fopen(full_path, "rb");
    if (!file) {
        const char *server_error = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\n\r\n"
                                   "<html><body><h1>500 Internal Server Error</h1></body></html>";
        send(client_socket, server_error, strlen(server_error), 0);
        return;
    }

    // Send HTTP response headers
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", content_type);
    send(client_socket, response, strlen(response), 0);

    // Send file content in chunks
    char file_buffer[BUFFER_SIZE];
    int bytes_read;
    while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(client_socket, file_buffer, bytes_read, 0);
    }
    fclose(file);
}

// Function to extract request details (method and filename)
void parse_request(const char *request, char *method, char *filename) {
    sscanf(request, "%s %s ", method, filename);

    // Handle requests for root ("/")
    if (strcmp(filename, "") == 0 || strcmp(filename, "/") == 0) {
        strcpy(filename, "/index.html");
    }
}

// Thread function to handle client request
DWORD WINAPI handle_client(LPVOID lpParam) {
    SOCKET client_socket = (SOCKET)lpParam;
    char buffer[BUFFER_SIZE];
    
    memset(buffer, 0, BUFFER_SIZE);
    recv(client_socket, buffer, BUFFER_SIZE, 0);
    printf("Received request:\n%s\n", buffer);

    char method[10] = {0}, filename[256] = {0};
    parse_request(buffer, method, filename);

    send_response(client_socket, filename, method);

    closesocket(client_socket);
    return 0;
}

int main() {
    WSADATA wsaData;
    SOCKET server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Socket creation failed.\n");
        WSACleanup();
        return 1;
    }

    // Set up server address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Bind failed.\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Listen for connections
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        printf("Listen failed.\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    printf("Server is running on port %d\n", PORT);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (client_socket == INVALID_SOCKET) {
            printf("Accept failed.\n");
            continue;
        }

        // Create a new thread to handle the client request
        DWORD threadId;
        CreateThread(NULL, 0, handle_client, (LPVOID)client_socket, 0, &threadId);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}