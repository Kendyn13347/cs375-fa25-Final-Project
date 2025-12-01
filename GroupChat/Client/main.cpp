#include "chat_client.h"
#include <iostream>
#include <thread>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../Shared/protocol.h"   // adjust case/path if needed
#include "../Shared/utils.h"      // for current_timestamp, send_all, recv_all


int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 8080;

    if (argc > 1) host = argv[1];
    if (argc > 2) port = std::stoi(argv[2]);

    ChatClient client(host, port);
    if (!client.connectToServer()) {
        std::cerr << "Failed to connect to " << host << ":" << port << "\n";
        return 1;
    }

    client.run();
    return 0;
}
