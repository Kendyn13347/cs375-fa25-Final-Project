#include "group_manager.h"
#include "../Shared/utils.h"
#include <algorithm>
#include <arpa/inet.h>

GroupManager::GroupManager(std::size_t cacheSize)
    : cacheSize_(cacheSize) {}

void GroupManager::joinGroup(uint16_t groupId, const ClientInfo& client) {
    std::lock_guard<std::mutex> lock(mtx_);
    groups_[groupId].push_back(client);
    if (!caches_.count(groupId)) {
        caches_.emplace(groupId, CircularCache<ChatPacket>(cacheSize_));
    }
}

void GroupManager::leaveGroup(uint16_t groupId, int socket) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = groups_.find(groupId);
    if (it == groups_.end()) return;
    auto& vec = it->second;
    vec.erase(std::remove_if(vec.begin(), vec.end(),
                             [socket](const ClientInfo& c) { return c.socket == socket; }),
              vec.end());
}

void GroupManager::broadcastToGroup(uint16_t groupId, const ChatPacket& packet) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = groups_.find(groupId);
    if (it == groups_.end()) return;

    // cache copy in host order
    caches_[groupId].push(packet);

    for (const auto& client : it->second) {
        send_all(client.socket, &packet, sizeof(ChatPacket));
    }
}

void GroupManager::sendRecentMessages(uint16_t groupId, int socket) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = caches_.find(groupId);
    if (it == caches_.end()) return;

    it->second.forEach([socket](const ChatPacket& pkt) {
        send_all(socket, &pkt, sizeof(ChatPacket));
    });
}

std::vector<uint16_t> GroupManager::listGroups() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<uint16_t> ids;
    ids.reserve(groups_.size());
    for (auto& kv : groups_) ids.push_back(kv.first);
    return ids;
}
