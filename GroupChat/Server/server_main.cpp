#include "chat_server.h"
#include <iostream>

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    ChatServer server(port, 4);
    server.run();
    return 0;
}
