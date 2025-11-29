#pragma once
#include <string>

class ChatClient {
public:
    ChatClient(const std::string& host, int port);
    bool connectToServer();
    void run();

private:
    std::string host_;
    int port_;
    int sock_;

    void receiverLoop();
};
