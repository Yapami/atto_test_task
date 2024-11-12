#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <fcntl.h>
#include <stdio.h>

typedef int socket_t;
constexpr int INVALID_SOCKET = -1;
constexpr int SOCKET_ERROR = -1;

inline void set_nonblocking(socket_t sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl F_GETFL failed");
        return;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl F_SETFL failed");
    }
}

void print_socket_error(const char* msg)
{
    perror(msg);
}

#endif // HELPERS_HPP