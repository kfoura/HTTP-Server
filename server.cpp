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

// ------------------ Response message helper functions ------------------

const std::string HTML_404(){
    return 
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: application/json\r\n"
        "Connection: keep-alive\r\n\r\n"
        "<html><body><h1>404 Not Found</h1></body></html>"; 
}

const std::string JSON_404(json payload){
    return 
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: application/json\r\n"
        "Connection: keep-alive\r\n\r\n" + 
         payload.dump();

}

const std::string JSON_200(json payload){
    return
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Connection: keep-alive\r\n\r\n" + 
        payload.dump();   
}

const std::string JSON_400(json payload){
    return
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Type: application/json\r\n"
        "Connection: keep-alive\r\n\r\n" + 
        payload.dump();
}
const std::string JSON_201(json payload){
    return
        "HTTP/1.1 201 Created\r\n"
        "Content-Type: application/json"
        "Location: /database\r\n"
        "Connection: keep-alive\r\n\r\n" + 
        payload.dump();
}

const std::string STATUS_204(){
    return 
        "HTTP/1.1 204 No Content\r\n"
        "Connection: keep-alive";
}

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
            "Connection: keep-alive\r\n"
            "\r\n"
            "<html><body><h1> Hello from my C++ HTTP server!</h1></body></html>";
        return response;
    } else if (route == "/database"){
        json j = read_json_file("database.json");
        std::string response;
        if (j.empty()){
            j = { {"error", "Database does not exist."} };
            response = JSON_404(j);
        } else {
            response = JSON_200(j);
        }
        return response;

    } else {
        return HTML_404();
    }
}

const std::string handle_POST(std::string route, json payload) {
    if (route == "/database"){
        if (payload.empty()){
            payload = {{"error", "The request body is empty. A POST request requires a payload."}};
            return JSON_400(payload);
        } else if (payload.contains("id")) {
            payload = {{"error", "The request body contains an id. IDs are automatically generated"}};
            return JSON_400(payload);
        } 
        else {
            json j = read_json_file("database.json");
            int curr_id = 1;

            if (j.is_array() && !j.empty()){
                int last_id = j.back()["id"];
                curr_id = last_id + 1;
            } 
            payload["id"] = curr_id;
            append_json_file("database.json", payload);
            return JSON_201(payload);
        }
    } else {
        return HTML_404();
    }   
}

const std::string handle_PUT(std::string route, json payload){
    if (route == "/database") {
        
        if (!payload.contains("id")){
            payload  = {{"error", "An id is needed for PUT requests."}};
            return JSON_400(payload);
        }
        int target_id = payload["id"];
        int index = 0;
        bool found = false;
        json j = read_json_file("database.json");
        for (const auto& element : j){
            if (element.contains("id") && element.at("id") == target_id){
                found = true;
                break;
            }
            index++;
        }
        if (found){
            j[index] = payload;
            write_json_file("database.json", j);
        } else {
            payload = {{"error", "This id does not exist in the database"}};
            return JSON_400(payload);
        }
        return JSON_200(payload);

    } else {
        return HTML_404();
    }
}

const std::string handle_PATCH(std::string route, json payload){
    if (route == "/database"){
        if (!payload.contains("id")){
            payload = {{"error", "An id is required for PATCH requests."}};
            return JSON_400(payload);
        }
        int target_id = payload["id"];
        int index = 0;
        bool found = false;
        json j = read_json_file("database.json");
        for (const auto& element : j) {
            if (element.contains("id") && element.at("id") == target_id){
                found = true;
                break;
            }
            index++;
        }
        if (found){
            for (const auto& item : payload.items()){
                 j[index][item.key()] = payload[item.key()];
                 write_json_file("database.json", j);
            }
        } else {
            payload = {{"error", "This id does not exist in the database."}};
            return JSON_400(payload);
        }
        payload = j[index];
        return JSON_200(payload);
    } else {
        return HTML_404();
    }
}

const std::string handle_DELETE(std::string route, json payload){
    if (route == "/database"){
        
        if (!payload.contains("id")){
            payload = {{"error", "You need to include an id for this DELETE request."}};
            return JSON_400(payload);
        }
        int target_id = payload["id"];
        int index = 0;
        bool found = false;
        json j = read_json_file("database.json");
        for (const auto& element : j){
            if (element.contains("id") && element.at("id") == target_id){
                found = true;
                break;
            }
            index++;
        }
        if (found){
            j.erase(j.begin() + index);
            write_json_file("database.json", j);
        } else{
            payload = {{"error", "This id does not exist in the database."}};
            return JSON_400(payload);
        }
        return STATUS_204();
    } else {
        return HTML_404();
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
        bool found_body = false;
        std::string line;
        std::string body;
        json payload;
        while (std::getline(ss, line)){
            if (line.empty() || line == "\r"){
                found_body = true;
                break;
            }
        }

        if (found_body){
            std::stringstream reqbody;
            reqbody << ss.rdbuf();
            body = reqbody.str();

            try {
                payload = json::parse(body);
                
            } catch (const json::parse_error& e){
                std::cerr << "JSON parsing error: " << e.what() << '\n';

            }
        } else {
            std::cerr << "Error: Could not find request body." << '\n';
        }
        std::string response;

        if (request_type == "GET"){
            response = handle_GET(route);
        } else if (request_type == "POST"){
            response = handle_POST(route, payload);
        } else if (request_type == "DELETE") {
            response = handle_DELETE(route, payload);
        } else if (request_type == "PUT") {
            response = handle_PUT(route, payload);
        } else if (request_type == "PATCH"){
            response = handle_PATCH(route, payload);
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