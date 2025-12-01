// Server/chat_server.cpp  (you can keep the name simple_server.cpp if you want)
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>

#include "thread_pool.h"
#include "group_manager.h"     // your group management
#include "../Shared/protocol.h"
#include "../Shared/utils.h"

GroupManager g_groups;         // global group manager (thread-safe inside)

// Handle a single client: stay in a loop processing ChatPackets
void handle_client(int client_socket) {
    std::string username = "anonymous";
    uint16_t currentGroup = 0;

    while (true) {
        ChatPacket pkt{};
        if (!recv_all(client_socket, &pkt, sizeof(pkt))) {
            std::cout << "Client disconnected.\n";
            if (currentGroup != 0) {
                g_groups.leaveGroup(currentGroup, client_socket);
            }
            close(client_socket);
            return;
        }

        uint16_t groupId = ntohs(pkt.groupID);
        uint32_t ts      = ntohl(pkt.timestamp);
        (void)ts; // unused for now

        switch (pkt.type) {
            case ChatType::JOIN: {
                username     = std::string(pkt.payload);
                currentGroup = groupId;

                std::cout << "Client joined group " << currentGroup
                          << " as '" << username << "'\n";

                g_groups.joinGroup(currentGroup,
                                   ClientInfo{client_socket, username});

                // send recent messages in that group back to this client
                g_groups.sendRecentMessages(currentGroup, client_socket);
                break;
            }

            case ChatType::MESSAGE: {
                // broadcast this packet to everyone in the group
                g_groups.broadcastToGroup(groupId, pkt);
                break;
            }

            case ChatType::LEAVE: {
                std::cout << "Client leaving group " << groupId << "\n";
                g_groups.leaveGroup(groupId, client_socket);
                close(client_socket);
                return; // end loop, close connection
            }

            case ChatType::LIST_GROUPS: {
                auto ids = g_groups.listGroups();
                for (uint16_t id : ids) {
                    ChatPacket resp{};
                    resp.type      = ChatType::SYSTEM;
                    resp.groupID   = htons(id);
                    resp.timestamp = htonl(current_timestamp());
                    std::snprintf(resp.payload, sizeof(resp.payload),
                                  "Active group: %u", id);
                    send_all(client_socket, &resp, sizeof(resp));
                }
                break;
            }

            default:
                // unknown type: ignore
                break;
        }
    }
}

int main() {
    stats_init();
    stats_init_vm(32);
    signal(SIGINT, handle_sigint);
    int server_fd, new_socket;
    struct sockaddr_in address{};
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 16) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    ThreadPool pool(4);

    std::cout << "Chat server listening on port 8080..." << std::endl;
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                (socklen_t *)&addrlen)) >= 0) {
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
