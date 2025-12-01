#pragma once

#include <string>
#include <cstdint>   // for std::uint16_t

class ChatClient {
public:
    ChatClient(const std::string& host, int port);

    // connect TCP socket
    bool connectToServer();

    // interactive loop (join group, send/receive messages)
    void run();

private:
    std::string  host_;
    int          port_;
    int          sock_;
    std::string  username_;
    std::uint16_t currentGroup_;   // <-- this is the group ID
    bool         running_;

    void receiverLoop();

    bool sendJoin(std::uint16_t groupId);
    bool sendLeave(std::uint16_t groupId);
    bool sendMessage(const std::string& text);
    bool sendListGroups();
};
