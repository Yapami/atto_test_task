#include "helpers.hpp"
#include "message.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <thread>
#include <unistd.h>

constexpr const char* SERVER_IP = "127.0.0.1";
constexpr int TCP_PORT = 9002;

int main()
{
    socket_t tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock == INVALID_SOCKET)
    {
        perror("TCP socket creation failed");
        return -1;
    }

    set_nonblocking(tcp_sock);

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TCP_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address / Address not supported");
        close(tcp_sock);
        return -1;
    }

    int result = connect(tcp_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
    if (result == SOCKET_ERROR)
    {
        if (errno != EINPROGRESS)
        {
            perror("TCP connect failed");
            close(tcp_sock);
            return -1;
        }

        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(tcp_sock, &writefds);
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        result = select(tcp_sock + 1, NULL, &writefds, NULL, &tv);
        if (result <= 0)
        {
            perror("TCP connect timeout or error");
            close(tcp_sock);
            return -1;
        }
    }

    std::cout << "Connected to server via TCP." << std::endl;

    while (true)
    {
        message msg{};
        ssize_t n = recv(tcp_sock, reinterpret_cast<char*>(&msg), sizeof(msg), 0);

        if (n > 0)
        {
            std::cout << "\033[32mReceived MessageId: " << msg.MessageId
                      << " with MessageData: " << msg.MessageData << "\033[0m" << std::endl;
        }
        else if (n == 0)
        {
            std::cout << "Server closed connection." << std::endl;
            break;
        }
        else
        {
            if (errno != EWOULDBLOCK && errno != EAGAIN)
            {
                perror("TCP recv failed");
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
    }

    close(tcp_sock);
    return 0;
}
