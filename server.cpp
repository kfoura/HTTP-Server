// Clean up some includes later
#include <iostream>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <sstream>
#include <string>
#include "json.hpp"
#include <fstream>
#include <ctime>
#include "threadpool.h"



using json = nlohmann::json;

// ------------------ JSON Helper Functions ------------------

json read_json_file(const std::string& filename) {
    std::ifstream input (filename);
    json j;
    if (input.is_open()){
        input >> j;
        input.close();
    } else {
        std::cerr << "Could not open file for reading: " << filename << '\n';
    }
    return j;
}

void write_json_file(const std::string& filename, const json& j){
    std::ofstream output (filename, std::ios::out | std::ios::trunc);

    if (!output) {
        std::cerr << "Could not open file for writing: " << filename << '\n';
        return;
    }

    output << j.dump(4);
    output.close();
}

void append_json_file(const std::string& filename, const json& new_object){
    json j;
    
    std::ifstream input(filename);
    if (input.is_open()){
        try {
            input >> j;
        } catch (const json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << "\nInitializing empty array. " << '\n';
            j = json::array();
        }
        input.close();
    } else {
        j = json::array();
    }

    if (!j.is_array()){
        j = json::array();
    }
    j.push_back(new_object);

    write_json_file(filename, j);
}

// ------------------ HTTP Server Proper ------------------

const std::string handle_GET(std::string route){
    if (route == "/"){
        const char* response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html><body><h1> Hello from my C++ HTTP server!</h1></body></html>";
        return response;
    } else if (route == "/database"){
        json j = read_json_file("database.json");
        std::string response;
        if (j.empty()){
            j = { {"error", "Database does not exist."} };
            response = 
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: application/json\r\n"
            "Connection: close\r\n\r\n" + 
            j.dump();
        } else {
            response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Connection: close\r\n\r\n" + 
            j.dump();
        }
        return response;

    } else {
        return 
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html><body><h1>404 Not Found</h1></body></html>";
    }
}

// Code for handling requests to the server
void parseRequest(int client_socket){
    // used to measure time
    auto start = std::chrono::high_resolution_clock::now();

    // buffer is used to store the incoming request, 4096 characters is usually enough to store all requests
    char buffer[4096];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0); // receives the request from the user and stores it into the buffer
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0'; // ends the buffer with the null terminator
        std::cout << "\nRequest: " << buffer << '\n';
        std::stringstream ss(buffer);
        std::string request_type = "";
        std::string route = "";
        ss >> request_type; ss >> route;
        std::string response;

        if (request_type == "GET"){
            response = handle_GET(route);
        }


        send(client_socket, response.c_str(), strlen(response.c_str()), 0); // returns this response 
        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        append_json_file("requests.json", {{"method", request_type}, {"route", route}, {"timestamp", std::ctime(&now)}});

    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "This request took " << duration.count() << " microseconds to process." << '\n';

    close(client_socket);
}

void setupSocket(int portNumber) {
    
    // create the serverside (my socket)
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket." << '\n';
        return;
    }
    // setup stuff to set the port and ip addressd
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    std::memset(&(serverAddress.sin_zero), 0, 8);
    
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1){
        std::cerr << "Failed to bind socket to port " << portNumber << '\n';
        close(serverSocket);
        return;
    }
    std::cout << "Socket successfully bound to port " << portNumber << '\n';
    if (listen(serverSocket, 5) == -1){
        std::cerr << "Failed to listen on socket" << '\n';
        close(serverSocket);
        return;
    }
    std::cout << "Server listening on port " << portNumber << "..." << '\n';
    // server is running
    ThreadPool pool(5);
    while (true){
        // accept requests
        int client_socket = accept(serverSocket, nullptr, nullptr);
        if (client_socket == -1){
            std::cerr << "Accept failed" << '\n';
            continue;
        }
        pool.enqueue([client_socket] {
            parseRequest(client_socket);
        });
    }


    close(serverSocket);
    return;
}



int main () {
    setupSocket(8080);
    return 0;
}