// Server/chat_server.cpp
#include "chat_server.h"
#include "../Shared/protocol.h"
#include "../Shared/utils.h"
#include "perf_stats.h"

#include <iostream>
#include <thread>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <atomic>

// -------- global run flag + signal handler for Ctrl+C --------

static std::atomic<bool> g_running{true};

void handle_sigint(int)
{
    // stop the accept loop
    g_running.store(false);

    std::cerr << "\nSIGINT received, dumping stats...\n";
    stats_dump_to_stdout();
    stats_dump_to_file("logs/performance.txt");

    std::_Exit(0);   // exit immediately after dumping stats
}

// -------- ChatServer implementation --------

ChatServer::ChatServer(int port, std::size_t workerThreads)
    : port_(port), server_fd_(-1), groups_(50), pool_(workerThreads) {}

ChatServer::~ChatServer() {
    if (server_fd_ >= 0) close(server_fd_);
}

void ChatServer::run() {
    // init performance stats
    stats_init();
    stats_init_vm(32);

    // install Ctrl+C handler (SIGINT)
    std::signal(SIGINT, handle_sigint);

    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        perror("socket");
        return;
    }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return;
    }

    if (listen(server_fd_, 16) < 0) {
        perror("listen");
        return;
    }

    std::cout << "Chat server listening on port " << port_ << "...\n";

    while (g_running.load()) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int clientSock = accept(server_fd_, (sockaddr*)&clientAddr, &len);
        if (clientSock < 0) {
            if (!g_running.load()) break;   // shutting down; stop cleanly
            perror("accept");
            continue;
        }

        std::thread(&ChatServer::handleClient, this, clientSock).detach();
    }

    // if we ever exit the loop normally, also dump stats here
    stats_dump_to_stdout();
    stats_dump_to_file("logs/performance.txt");
}

void ChatServer::handleClient(int clientSock) {
    std::string username = "anonymous";
    uint16_t currentGroup = 1;

    while (true) {
        ChatPacket pkt{};
        if (!recv_all(clientSock, &pkt, sizeof(ChatPacket))) {
            std::cout << "Client disconnected.\n";
            groups_.leaveGroup(currentGroup, clientSock);
            close(clientSock);
            return;
        }

        // convert fields from network order
        uint16_t groupId = ntohs(pkt.groupID);
        uint32_t ts      = ntohl(pkt.timestamp);
        (void)ts; // currently unused

        switch (pkt.type) {
            case ChatType::JOIN: {
                username = std::string(pkt.payload);
                currentGroup = groupId;
                groups_.joinGroup(groupId, ClientInfo{clientSock, username});
                groups_.sendRecentMessages(groupId, clientSock);
                break;
            }
            case ChatType::MESSAGE: {
                // track stats
                stats_record_message();
                stats_vm_access(groupId);

                // broadcast via thread pool
                pool_.enqueue([this, pkt, groupId]() {
                    ChatPacket toSend = pkt;
                    // timestamp on server
                    toSend.timestamp = htonl(current_timestamp());
                    groups_.broadcastToGroup(groupId, toSend);
                });
                break;
            }
            case ChatType::LEAVE: {
                groups_.leaveGroup(groupId, clientSock);
                close(clientSock);
                return;
            }
            case ChatType::LIST_GROUPS: {
                auto ids = groups_.listGroups();
                for (uint16_t id : ids) {
                    ChatPacket resp{};
                    resp.type = ChatType::SYSTEM;
                    resp.groupID = htons(id);
                    std::snprintf(resp.payload, sizeof(resp.payload),
                                  "Active group: %u", id);
                    resp.timestamp = htonl(current_timestamp());
                    send_all(clientSock, &resp, sizeof(resp));
                }
                break;
            }
            default:
                break;
        }
    }
}
