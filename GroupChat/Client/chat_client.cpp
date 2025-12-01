#include "chat_client.h"

#include <iostream>
#include <thread>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../Shared/protocol.h"  // adjust case/path if needed
#include "../Shared/utils.h"

using std::uint16_t;  // convenience alias

ChatClient::ChatClient(const std::string& host, int port)
    : host_(host),
      port_(port),
      sock_(-1),
      username_(),
      currentGroup_(0),
      running_(false) {}

bool ChatClient::connectToServer() {
    sock_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0) {
        perror("socket");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<uint16_t>(port_));

    if (inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported\n";
        return false;
    }

    if (connect(sock_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return false;
    }

    return true;
}

bool ChatClient::sendJoin(uint16_t groupId) {
    ChatPacket pkt{};
    pkt.type      = ChatType::JOIN;
    pkt.groupID   = htons(groupId);
    pkt.timestamp = htonl(current_timestamp());
    std::snprintf(pkt.payload, sizeof(pkt.payload), "%s", username_.c_str());
    return send_all(sock_, &pkt, sizeof(pkt));
}

bool ChatClient::sendLeave(uint16_t groupId) {
    ChatPacket pkt{};
    pkt.type      = ChatType::LEAVE;
    pkt.groupID   = htons(groupId);
    pkt.timestamp = htonl(current_timestamp());
    pkt.payload[0] = '\0';
    return send_all(sock_, &pkt, sizeof(pkt));
}

bool ChatClient::sendMessage(const std::string& text) {
    ChatPacket pkt{};
    pkt.type      = ChatType::MESSAGE;
    pkt.groupID   = htons(currentGroup_);
    pkt.timestamp = htonl(current_timestamp());
    std::snprintf(pkt.payload, sizeof(pkt.payload), "%s: %s",
                  username_.c_str(), text.c_str());
    return send_all(sock_, &pkt, sizeof(pkt));
}

bool ChatClient::sendListGroups() {
    ChatPacket pkt{};
    pkt.type      = ChatType::LIST_GROUPS;
    pkt.groupID   = htons(currentGroup_);
    pkt.timestamp = htonl(current_timestamp());
    pkt.payload[0] = '\0';
    return send_all(sock_, &pkt, sizeof(pkt));
}

// Background thread: reads packets from server and prints them.
void ChatClient::receiverLoop() {
    while (running_) {
        ChatPacket pkt{};
        if (!recv_all(sock_, &pkt, sizeof(pkt))) {
            std::cout << "Disconnected from server.\n";
            running_ = false;
            ::close(sock_);
            sock_ = -1;
            return;
        }

        uint16_t groupId = ntohs(pkt.groupID);
        uint32_t ts      = ntohl(pkt.timestamp);
        (void)ts; // timestamp available if you want to print it

        std::string text(pkt.payload);

        if (pkt.type == ChatType::SYSTEM) {
            std::cout << "[SYSTEM] " << text << "\n";
        } else if (pkt.type == ChatType::MESSAGE) {
            std::cout << "[Group " << groupId << "] " << text << "\n";
        } else {
            std::cout << "[INFO] " << text << "\n";
        }
    }
}

void ChatClient::run() {
    // ---- Ask for username and group ID ----
    std::cout << "Enter username: ";
    std::getline(std::cin, username_);
    if (username_.empty()) username_ = "anonymous";

    std::cout << "Enter group ID to join (number): ";
    std::string groupStr;
    std::getline(std::cin, groupStr);
    if (groupStr.empty()) groupStr = "1";
    currentGroup_ = static_cast<uint16_t>(std::stoi(groupStr));

    if (!sendJoin(currentGroup_)) {
        std::cerr << "Failed to send JOIN packet.\n";
        return;
    }

    std::cout << "Joined group " << currentGroup_
              << " as '" << username_ << "'.\n";
    std::cout << "Commands: /groups, /quit\n";

    running_ = true;
    std::thread recvThread(&ChatClient::receiverLoop, this);
    recvThread.detach();

    // ---- Main input loop ----
    while (running_) {
        std::string line;
        if (!std::getline(std::cin, line)) {
            break;  // EOF
        }

        if (line == "/quit") {
            sendLeave(currentGroup_);
            running_ = false;
            break;
        } else if (line == "/groups") {
            sendListGroups();
        } else if (!line.empty()) {
            sendMessage(line);
        }
    }

    running_ = false;
    if (sock_ >= 0) {
        ::close(sock_);
        sock_ = -1;
    }

    std::cout << "Client exiting.\n";
}
