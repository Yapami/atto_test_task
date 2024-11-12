#include "helpers.hpp"
#include "message.hpp"
#include "message_map.hpp"
#include "message_queue.hpp"
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <thread>
#include <unistd.h>

constexpr int UDP_PORT = 9000;
constexpr int TCP_PORT = 9002;

MessageMap message_map;
socket_t tcp_client_sock = INVALID_SOCKET;
std::atomic<bool> tcp_client_connected(false);
MessageQueue send_queue;

void udp_receiver(int thread_id)
{
    socket_t udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock == INVALID_SOCKET)
    {
        perror("UDP socket creation failed");
        return;
    }

    set_nonblocking(udp_sock);

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(UDP_PORT + thread_id);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
    {
        perror("UDP bind failed");
        close(udp_sock);
        return;
    }

    char buffer[sizeof(message)];
    while (true)
    {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        ssize_t n =
            recvfrom(udp_sock, buffer, sizeof(buffer), 0, (sockaddr*)&client_addr, &addr_len);
        if (n > 0)
        {
            message msg;
            memcpy(&msg, buffer, sizeof(message));

            bool is_new_message = message_map.exists(msg.MessageId) == false;
            if (is_new_message)
            {
                std::cout << (msg.MessageData == 10 ? "\033[32m" : "") << "Thread " << thread_id
                          << " received MessageId: " << msg.MessageId
                          << " to TCP client. MessageData: " << msg.MessageData << "\033[0m"
                          << std::endl;

                if (msg.MessageData == 10)
                {
                    send_queue.enqueue(msg);
                }
                message_map.insert(msg);
            }
            else
            {
                std::cout << "\033[33mThread " << thread_id
                          << " received MessageId: " << msg.MessageId
                          << "; MessageData: " << msg.MessageData << ", but this is a duplicate"
                          << "\033[0m" << std::endl;
            }
        }
        else if (n == SOCKET_ERROR)
        {
            if (errno != EWOULDBLOCK && errno != EAGAIN)
            {
                perror("UDP recvfrom failed");
                close(udp_sock);
                return;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    close(udp_sock);
}

void tcp_sender()
{
    socket_t tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock == INVALID_SOCKET)
    {
        perror("TCP socket creation failed");
        return;
    }

    set_nonblocking(tcp_sock);

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TCP_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(tcp_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
    {
        perror("TCP bind failed");
        close(tcp_sock);
        return;
    }

    listen(tcp_sock, 1);
    std::cout << "Waiting for TCP client to connect..." << std::endl;

    while (true)
    {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        tcp_client_sock = accept(tcp_sock, (sockaddr*)&client_addr, &addr_len);
        if (tcp_client_sock != INVALID_SOCKET)
        {
            set_nonblocking(tcp_client_sock);
            std::cout << "TCP client connected." << std::endl;
            tcp_client_connected.store(true);

            while (tcp_client_connected.load())
            {
                message msg;
                while (send_queue.dequeue(msg))
                {
                    ssize_t sent = send(tcp_client_sock, (char*)&msg, sizeof(msg), 0);
                    if (sent == SOCKET_ERROR)
                    {
                        if (errno != EWOULDBLOCK && errno != EAGAIN)
                        {
                            perror("TCP send failed");
                            close(tcp_client_sock);
                            tcp_client_connected.store(false);
                            break;
                        }
                    }
                    else
                    {
                        std::cout << "\033[32mSent MessageId " << msg.MessageId
                                  << " to TCP client. MessageData: " << msg.MessageData << "\033[0m"
                                  << std::endl;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            close(tcp_client_sock);
            std::cout << "TCP client disconnected." << std::endl;
        }
        else
        {
            if (errno != EWOULDBLOCK && errno != EAGAIN)
            {
                perror("TCP accept failed");
                close(tcp_sock);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    close(tcp_sock);
}

int main()
{
    std::thread udp_thread1(udp_receiver, 0);
    std::thread udp_thread2(udp_receiver, 1);
    std::thread tcp_thread(tcp_sender);

    udp_thread1.join();
    udp_thread2.join();
    tcp_thread.join();

    return 0;
}
