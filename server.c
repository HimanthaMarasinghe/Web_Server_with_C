#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to determine content type based on file extension
const char *get_content_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";  // Default binary type

    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".mp4") == 0) return "video/mp4";
    if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";

    return "application/octet-stream";  // Default for unknown types
}

// Function to send response
void send_response(SOCKET client_socket, const char *file_path) {
    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "./root/%s", file_path);
    FILE *file = fopen(full_path, "rb");
    char response[BUFFER_SIZE];

    if (!file) {
        // 404 Not Found Response
        const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
                                "<html><body><h1>404 Not Found</h1></body></html>";
        send(client_socket, not_found, strlen(not_found), 0);
    } else {
        // Get content type
        const char *content_type = get_content_type(file_path);

        // Send HTTP response headers
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
}

// Function to extract file name from HTTP request
void extract_filename(const char *request, char *filename) {
    sscanf(request, "GET /%s ", filename);

    // If only "/" is requested, serve index.html
    if (strcmp(filename, "") == 0 || strcmp(filename, "/") == 0) {
        strcpy(filename, "index.html");
    }
}

int main() {
    WSADATA wsaData;
    SOCKET server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Initialize Winsock
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    printf("Server is running on port %d\n", PORT);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        recv(client_socket, buffer, BUFFER_SIZE, 0);
        printf("Received request:\n%s\n", buffer);

        char filename[256] = {0};
        extract_filename(buffer, filename);

        send_response(client_socket, filename);
        closesocket(client_socket);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}