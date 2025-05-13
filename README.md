# C Web Server (Windows)

A simple multithreaded HTTP server written in C for Windows. It serves static files (HTML, CSS, JS, images, etc.) from a `root` directory.

## 🔧 Features

- Handles basic HTTP `GET` requests
- Returns 404, 403, 405, 415, and 500 error responses where appropriate
- Uses Windows sockets (Winsock2)
- Supports multiple clients via threads
- Determines content type by file extension

## 📋 Requirements

- Windows OS
- GCC compiler for Windows (e.g., MinGW, TDM-GCC)
- Winsock2 (included in Windows)

## 🛠️ How to Compile

Use `gcc` with the Winsock2 library:

```bash
gcc server.c -o server.exe -lws2_32
```

## 🚀 How to run

1. Add your static files (like index.html, style.css, etc.) into the root folder.
2. Run the server.
```
./server.exe
```
3.Open your browser and go to http://localhost:8080/
