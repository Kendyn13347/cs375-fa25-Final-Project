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
