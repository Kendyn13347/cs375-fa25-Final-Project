// server/simple_server.cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include "thread_pool.h"

void handle_client(int client_socket) {
    char buffer[1024] = {0};
    read(client_socket, buffer, 1024);
    std::cout << "Message from client: " << buffer << std::endl;
    std::string response = "Server echo: " + std::string(buffer);
    send(client_socket, response.c_str(), response.length(), 0);
    close(client_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    ThreadPool pool(4);

    std::cout << "Server listening on port 8080..." << std::endl;
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                (socklen_t *)&addrlen))) {
        std::cout << "New connection accepted" << std::endl;
        pool.enqueue([new_socket]() { handle_client(new_socket); });
    }

    if (new_socket < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }

    close(server_fd);
    return 0;
}
