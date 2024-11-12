cmake_minimum_required(VERSION 3.10)
project(atto_test_task)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

add_executable(server server.cpp)
target_link_libraries(server Threads::Threads)

add_executable(udp_client udp_client.cpp)
add_executable(tcp_client tcp_client.cpp)
