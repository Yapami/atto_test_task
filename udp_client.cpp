#include "helpers.hpp"
#include "message.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <iostream>
#include <netinet/in.h>
#include <random>
#include <thread>
#include <unistd.h>

constexpr const char* SERVER_IP = "127.0.0.1";
constexpr int UDP_PORT = 9000;

int main(int argc, char* argv[])
{
    int client_id = 0;

    if (argc > 1)
    {
        client_id = std::atoi(argv[1]);
    }

    socket_t udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock == INVALID_SOCKET)
    {
        perror("UDP socket creation failed");
        return -1;
    }

    set_nonblocking(udp_sock);

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(UDP_PORT + (client_id % 2));
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address / Address not supported");
        close(udp_sock);
        return -1;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(1, 1000);

    while (true)
    {
        message msg{};
        msg.MessageSize = sizeof(message);
        msg.MessageType = 1;
        msg.MessageId = dis(gen);
        msg.MessageData = dis(gen) % 15;

        ssize_t n = sendto(udp_sock, reinterpret_cast<char*>(&msg), sizeof(msg), 0,
                           reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr));
        if (n == SOCKET_ERROR)
        {
            if (errno != EWOULDBLOCK && errno != EAGAIN)
            {
                perror("UDP sendto failed");
            }
        }
        else
        {
            std::cout << (msg.MessageData == 10 ? "\033[32m" : "") << "UDP Client " << client_id
                      << " sent MessageId: " << msg.MessageId
                      << "; MessageData: " << msg.MessageData << "\033[0m" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    close(udp_sock);
    return 0;
}
