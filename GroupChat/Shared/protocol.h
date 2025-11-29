#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct ChatPacket {
    uint8_t  type;        // 0 = JOIN, 1 = MESSAGE, 2 = LEAVE, 3 = LIST_GROUPS, 4 = SYSTEM
    uint16_t groupID;     // network order on the wire
    uint32_t timestamp;   // epoch seconds, network order
    char     payload[256];// UTF-8 text, null-terminated if shorter than 256
};
#pragma pack(pop)

// Packet type helpers (just constants)
namespace ChatType {
    constexpr uint8_t JOIN       = 0;
    constexpr uint8_t MESSAGE    = 1;
    constexpr uint8_t LEAVE      = 2;
    constexpr uint8_t LIST_GROUPS= 3;
    constexpr uint8_t SYSTEM     = 4;
}
