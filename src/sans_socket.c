#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "include/sans.h"
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>

int sans_connect(const char* host, int port, int protocol) {
        struct addrinfo hints, *result, *p;
        int sockfd, status;
        char port_str[6];

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        snprintf(port_str, sizeof(port_str), "%d", port);

        if ((status = getaddrinfo(host, port_str, &hints, &result)) != 0) {
            return -2;
        }

        for (p = result; p != NULL; p = p->ai_next) {
            sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sockfd == -1) {
                continue;
            }

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) != -1) {
                break;
            }

            close(sockfd);
        }

        if (p == NULL) {
            return -3;
        }

        return sockfd;


}

int sans_accept(const char* addr, int port, int protocol){
        struct addrinfo hints, *result, *p;
        int sockfd = -1, new_socket, status;
        char port_str[6];

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        snprintf(port_str, sizeof(port_str), "%d", port);

        if ((status = getaddrinfo(addr, port_str, &hints, &result)) != 0) {
            return -2;
        }

        for (p = result; p != NULL; p = p->ai_next) {
            sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sockfd == -1) {
                continue;
            }

            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                continue;
            }

            if (listen(sockfd, 5) == -1) {
                close(sockfd);
                continue;
            }

            break;
        }

        if (p == NULL) {
            return -3;
        }

        new_socket = accept(sockfd, NULL, NULL);
        if (new_socket == -1) {
            close(sockfd);
            return -4;
        }

        close(sockfd);
        return new_socket;

}

int sans_disconnect(int socket) {
    return close(socket);  // Close the socket
}