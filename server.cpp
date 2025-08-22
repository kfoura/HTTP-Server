#include <iostream>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>


// Code for handling requests to the server
void parseRequest(int client_socket){
    // used to measure time
    auto start = std::chrono::high_resolution_clock::now();

    // buffer is used to store the incoming request, 4096 characters is usually enough to store all requests
    char buffer[4096];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0); // receives the request from the user and stores it into the buffer
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0'; // ends the buffer with the null terminator
        std::cout << "\nRequest: " << buffer << std::endl;


        const char* response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html><body><h1>Hello World!</h1></body></html>";

        send(client_socket, response, strlen(response), 0); // returns this response 
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "This request took " << duration.count() << " microseconds to process." << std::endl;

    close(client_socket);
}

void setupSocket(int portNumber) {
    // create the serverside (my socket)
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return;
    }
    // setup stuff to set the port and ip addressd
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    std::memset(&(serverAddress.sin_zero), 0, 8);
    
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1){
        std::cerr << "Failed to bind socket to port " << portNumber << std::endl;
        close(serverSocket);
        return;
    }
    std::cout << "Socket successfully bound to port " << portNumber << std::endl;
    if (listen(serverSocket, 5) == -1){
        std::cerr << "Failed to listen on socket" << std::endl;
        close(serverSocket);
        return;
    }
    std::cout << "Server listening on port " << portNumber << "..." << std::endl;
    // server is running
    while (true){
        // accept requests
        int client_socket = accept(serverSocket, nullptr, nullptr);
        if (client_socket == -1){
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        parseRequest(client_socket);
    }


    close(serverSocket);
    return;
}



int main () {
    setupSocket(8080);
    return 0;
}