# GroupChat — Multi-Group Client–Server Chat (C++)

A simple TCP-based chat server and CLI client supporting multiple chat groups.

## Features
- TCP socket server and client (IPv4)
- Multiple groups: clients join a group and receive broadcasts for that group
- Simple binary `ChatPacket` protocol (fixed-size 1+2+4+256 bytes)
- Server-side thread pool for broadcast tasks and recent-message cache per group
- Minimal CLI client with background receiver thread

## Requirements
- Linux, macOS, or other POSIX environment
- C++17-compatible compiler (g++, clang++)
- CMake 3.10+ and make

## Quick Build
From the `GroupChat` directory:

```bash
mkdir -p build && cd build
cmake ..
make -j
```

This places compiled binaries in the source subdirectories (e.g. `Server/chat_server`, `Client/chat_client`) according to the CMake configuration.

## Running

- Start the server (default port 8080):

```bash
./Server/chat_server        # optional: ./Server/chat_server 9090
```

- Start a client (connects to 127.0.0.1:8080 by default):

```bash
./Client/chat_client       # optional: ./Client/chat_client 127.0.0.1 8080
```

## Client usage
- On start the client prompts for a username and a group ID to join.
- Commands available while running:
  - `/groups` — request a list of active groups from the server
  - `/quit`   — leave the current group and exit

## Protocol (brief)
- `ChatPacket` (packed struct):
  - `uint8_t  type`     — packet type (JOIN, MESSAGE, LEAVE, LIST_GROUPS, SYSTEM)
  - `uint16_t groupID`  — group id (network byte order)
  - `uint32_t timestamp`— epoch seconds (network byte order)
  - `char payload[256]` — UTF-8 text (null-terminated if shorter)

See `Shared/protocol.h` and `Shared/utils.h` for implementation details.

## Repository layout
- `Server/` — server sources, `chat_server` binary
- `Client/` — client sources, `chat_client` binary
- `Shared/` — shared headers (`protocol.h`, `utils.h`, etc.)
- `Logs/` — runtime logs and performance dumps
- `Tests/` — unit / integration test sources

## Troubleshooting
- If binding fails, ensure the chosen port is free and you have permission to bind to it.
- If clients immediately disconnect, check that server is running and listening on the expected port.

## Testing
- There is a small test file at `Tests/bot_tests.cpp`. You can compile and run tests manually or extend the CMake setup to add test targets.

## Credits & License
- Course project for CS375. See source headers for any attribution or license comments.

If you'd like, I can also:
- add a `build/bin` install step in CMake to collect binaries
- add a small example script to launch a server + multiple clients for demos
# GroupChat – Multi-Group Client-Server Chat (C++)

Features:
- TCP socket-based chat server
- Multiple chat groups, clients can join and broadcast
- Binary `ChatPacket` protocol
- Thread pool on server for message broadcast tasks
- Recent message cache using circular buffer per group
- Simple CLI client with send/receive threads

Build:

```bash
mkdir build && cd build
cmake ..
make
