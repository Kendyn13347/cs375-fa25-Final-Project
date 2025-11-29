#pragma once
#include <cstdint>
#include <string>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>

inline uint32_t current_timestamp() {
    using namespace std::chrono;
    return static_cast<uint32_t>(
        duration_cast<seconds>(system_clock::now().time_since_epoch()).count()
    );
}

// send all bytes in buffer
inline bool send_all(int sock, const void* data, std::size_t len) {
    const char* ptr = static_cast<const char*>(data);
    while (len > 0) {
        ssize_t sent = ::send(sock, ptr, len, 0);
        if (sent <= 0) return false;
        ptr += sent;
        len -= static_cast<std::size_t>(sent);
    }
    return true;
}

// recv exactly len bytes (unless peer closes)
inline bool recv_all(int sock, void* data, std::size_t len) {
    char* ptr = static_cast<char*>(data);
    while (len > 0) {
        ssize_t recvd = ::recv(sock, ptr, len, 0);
        if (recvd <= 0) return false;
        ptr += recvd;
        len -= static_cast<std::size_t>(recvd);
    }
    return true;
}
