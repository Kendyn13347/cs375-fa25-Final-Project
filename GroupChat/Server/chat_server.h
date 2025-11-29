#pragma once
#include "group_manager.h"
#include "thread_pool.h"

class ChatServer {
public:
    ChatServer(int port, std::size_t workerThreads = 4);
    ~ChatServer();

    void run();     // blocking accept loop

private:
    int port_;
    int server_fd_;
    GroupManager groups_;
    ThreadPool pool_;

    void handleClient(int clientSock);
};
