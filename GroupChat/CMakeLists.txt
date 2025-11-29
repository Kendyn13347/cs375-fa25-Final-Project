cmake_minimum_required(VERSION 3.10)
project(GroupChat CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(shared)

add_executable(chat_server
    server/main.cpp
    server/chat_server.cpp
    server/group_manager.cpp
    server/thread_pool.cpp
)

target_link_libraries(chat_server pthread)

add_executable(chat_client
    client/main.cpp
    client/chat_client.cpp
)

target_link_libraries(chat_client pthread)
