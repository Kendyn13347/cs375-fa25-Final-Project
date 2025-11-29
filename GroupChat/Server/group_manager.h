#pragma once
#include <map>
#include <vector>
#include <string>
#include <mutex>
#include "../shared/protocol.h"
#include "../shared/cache.h"

struct ClientInfo {
    int socket;
    std::string name;
};

class GroupManager {
public:
    explicit GroupManager(std::size_t cacheSize = 50);

    void joinGroup(uint16_t groupId, const ClientInfo& client);
    void leaveGroup(uint16_t groupId, int socket);
    void broadcastToGroup(uint16_t groupId, const ChatPacket& packet);
    void sendRecentMessages(uint16_t groupId, int socket);

    std::vector<uint16_t> listGroups() const;

private:
    mutable std::mutex mtx_;
    std::map<uint16_t, std::vector<ClientInfo>> groups_;
    std::map<uint16_t, CircularCache<ChatPacket>> caches_;
    std::size_t cacheSize_;
};
