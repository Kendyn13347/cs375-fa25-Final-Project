#include "chat_client.h"
#include "../shared/protocol.h"
#include "../shared/utils.h"

#include <iostream>
#include <thread>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

ChatClient::ChatClient(const std::string& host, int port)
    : host_(host), port_(port), sock_(-1) {}

bool ChatClient::connectToServer() {
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0) {
        perror("socket");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
        return false;
    }

    if (connect(sock_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return false;
    }
    return true;
}

void ChatClient::receiverLoop() {
    while (true) {
        ChatPacket pkt{};
        if (!recv_all(sock_, &pkt, sizeof(ChatPacket))) {
            std::cout << "Disconnected from server.\n";
            ::close(sock_);
            sock_ = -1;
            return;
        }
        uint32_t ts = ntohl(pkt.timestamp);
        (void)ts;
        std::string text(pkt.payload);

        if (pkt.type == ChatType::SYSTEM) {
            std::cout << "[SYSTEM] " << text << "\n";
        } else {
            std::cout << "[Group " << ntohs(pkt.groupID) << "] "
                      << text << "\n";
        }
    }
}

void ChatClient::run() {
    std::string username;
    std::cout << "Enter username: ";
    std::getline(std::cin, username);

    uint16_t groupId = 1;
    std::cout << "Enter group ID to join (number): ";
    std::string tmp;
    std::getline(std::cin, tmp);
    if (!tmp.empty()) groupId = static_cast<uint16_t>(std::stoi(tmp));

    // send JOIN
    ChatPacket join{};
    join.type = ChatType::JOIN;
    join.groupID = htons(groupId);
    join.timestamp = htonl(current_timestamp());
    std::snprintf(join.payload, sizeof(join.payload), "%s", username.c_str());
    send_all(sock_, &join, sizeof(join));

    std::thread recvThread(&ChatClient::receiverLoop, this);
    recvThread.detach();

    std::cout << "Type messages. /quit to exit.\n";
    while (true) {
        std::string line;
        std::getline(std::cin, line);
        if (line == "/quit") {
            ChatPacket leave{};
            leave.type = ChatType::LEAVE;
            leave.groupID = htons(groupId);
            leave.timestamp = htonl(current_timestamp());
            send_all(sock_, &leave, sizeof(leave));
            ::close(sock_);
            return;
        } else if (line == "/groups") {
            ChatPacket list{};
            list.type = ChatType::LIST_GROUPS;
            list.groupID = htons(groupId);
            list.timestamp = htonl(current_timestamp());
            send_all(sock_, &list, sizeof(list));
        } else {
            ChatPacket msg{};
            msg.type = ChatType::MESSAGE;
            msg.groupID = htons(groupId);
            msg.timestamp = htonl(current_timestamp());
            std::snprintf(msg.payload, sizeof(msg.payload), "%s: %s",
                          username.c_str(), line.c_str());
            send_all(sock_, &msg, sizeof(msg));
        }
    }
}

/* #include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>   // required for inet_pton
#include <unistd.h>
#include <string.h>

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);   // connect to server on port 8080

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "Invalid address / Address not supported" << std::endl;
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Connection failed" << std::endl;
        return -1;
    }

    std::string message;
    std::cout << "Enter message: ";
    std::getline(std::cin, message);

    // Send message to server
    send(sock, message.c_str(), message.length(), 0);

    // Read server response
    read(sock, buffer, 1024);
    std::cout << "Server response: " << buffer << std::endl;

    close(sock);
    return 0;
}
*/