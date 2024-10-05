#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include "include/sans.h"
#include "include/rudp.h"
#define RUDP_HANDSHAKE_TIMEOUT_MS 20
#define MAX_TRIES 4
#define MAX_ITERATIONS 100

typedef struct {
    int sfd;
    struct sockaddr_in addr;
    size_t seq_num;
} connection_rudp_t;
connection_rudp_t connection_rudp;

//static void set_socket_timeout(int sockfd) {
//    struct timeval timeout = {
//            .tv_sec = 0,
//            .tv_usec = 20000
//    };
//    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
//}

int sans_connect(const char* host, int port, int protocol) {
    printf("sans connect\n");
    if(protocol != IPPROTO_TCP && protocol != IPPROTO_RUDP){
        return -1;
    }
    if (protocol == IPPROTO_RUDP) {
        struct addrinfo hints, *res, *p;
        int sfd, status;
        char port_str[6];

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        snprintf(port_str, sizeof(port_str), "%d", port);

        if ((status = getaddrinfo(host, port_str, &hints, &res)) != 0) {
            return -2;
        }

        for (p = res; p != NULL; p = p->ai_next) {
            sfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if(sfd == -1)
                continue;
            struct sockaddr *server_addr = p->ai_addr;
            rudp_packet_t syn_pkt = {.type = SYN, .seqnum = 0};  // Initialize SYN packet
            rudp_packet_t ack_pkt = {0};  // Initialize ACK packet
            int retries = 0;

            while (retries < MAX_TRIES) {
                printf("Sending SYN packet\n");
                sendto(sfd, &syn_pkt, sizeof(syn_pkt), 0, server_addr, sizeof(*server_addr));

                socklen_t addr_len = sizeof(*server_addr);
                int recv_result = recvfrom(sfd, &ack_pkt, sizeof(ack_pkt), 0, server_addr,
                                           &addr_len);
                if (recv_result > 0 && ack_pkt.type == (SYN | ACK)) {
                    rudp_packet_t ack_response = {.type = ACK, .seqnum = ack_pkt.seqnum};  // Initialize ACK response
                    sendto(sfd, &ack_response, sizeof(ack_response), 0, server_addr,
                           sizeof(*server_addr));
                    printf("Received SYN+ACK and sent ACK\n");

                    connection_rudp.sfd = sfd;
                    connection_rudp.addr = *(struct sockaddr_in *) server_addr;
                    return sfd;
                } else {
                    printf("Timeout or incorrect packet, retransmitting SYN\n");
                }
                retries++;
            }
            close(sfd);
        }
        return -1;
    }

    else{
        //TCP implementation
        struct addrinfo *addr_info_result;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        char port_str[6]; // Maximum of 5 digits + null terminator
        snprintf(port_str, sizeof(port_str), "%d", port);
        if (getaddrinfo(host, port_str, &hints, &addr_info_result) != 0)
            return -2;
        struct addrinfo *result = addr_info_result;
        int sfd = -1; // socket file descriptor
        for (result = addr_info_result; result != NULL; result = result->ai_next) {
            sfd = socket(result->ai_family, result->ai_socktype,
                         result->ai_protocol);
            if (sfd == -1)
                continue;
            if (connect(sfd, result->ai_addr, result->ai_addrlen) == 0)
                break; // Success
            close(sfd); // Close on failure
            sfd = -1;
        }
        return sfd;

    }

}

int rudp_accept(int sfd){
    int tries = 0;
    rudp_packet_t  packet;
    rudp_packet_t  syn_ack_packet = { .type = SYN | ACK };
    rudp_packet_t  ack_packet = { .type = 0 };
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    while(tries++ < MAX_TRIES) {
        int n_recvd_bytes = recvfrom(sfd, &packet, sizeof(packet), 0, (struct sockaddr*)&sender_addr, &sender_addr_len);
        if(n_recvd_bytes > 0) {
            if(packet.type == SYN) {
                while (tries++ < MAX_TRIES) {
                    sendto(sfd, &syn_ack_packet, sizeof(syn_ack_packet), 0, (struct sockaddr *) &sender_addr, sender_addr_len);
                    if (recvfrom(sfd, &ack_packet, sizeof(ack_packet), 0, (struct sockaddr *) &sender_addr, &sender_addr_len) <= 0)
                        printf("Timed out...\n");
                    connection_rudp.sfd = sfd;
                    connection_rudp.addr = sender_addr;
                    if (ack_packet.type == ACK)
                        return sfd;
                }
                break;
            }
        }
    }
    close(sfd);
    return -1;
}
int sans_accept(const char* addr, int port, int protocol) {
    printf("sans_accept\n");
    if(protocol != IPPROTO_TCP && protocol != IPPROTO_RUDP){
        return -1;
    }
    if (protocol == IPPROTO_RUDP) {
        struct addrinfo *addr_info_result;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
        hints.ai_socktype = SOCK_DGRAM; // UDP sockets
        hints.ai_protocol = IPPROTO_UDP; // Only UDP protocol
        char port_str[6]; // Maximum of 5 digits + null terminator
        snprintf(port_str, sizeof(port_str), "%d", port);
        if (getaddrinfo(addr, port_str, &hints, &addr_info_result) != 0)
            return -2;
        struct addrinfo *result = addr_info_result;
        int sfd = -1; // socket file descriptor
        for (result = addr_info_result; result != NULL; result = result->ai_next) {
            sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
            if (sfd == -1)
                continue;
            if (bind(sfd, result->ai_addr, result->ai_addrlen) == 0)
                break;
            close(sfd);
            sfd = -1;
        }
        if (sfd == -1)
            return -2;
        return rudp_accept(sfd);
    }
    else{
        struct addrinfo *addr_info_result;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
        hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
        hints.ai_protocol = IPPROTO_TCP; // Only TCP protocol
        char port_str[6]; // Maximum of 5 digits + null terminator
        snprintf(port_str, sizeof(port_str), "%d", port);
        if (getaddrinfo(addr, port_str, &hints, &addr_info_result) != 0)
            return -2;
        struct addrinfo *result = addr_info_result;
        int sfd = -1; // socket file descriptor
        for (result = addr_info_result; result != NULL; result = result->ai_next) {
            sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
            if (sfd == -1)
                continue;
            if (bind(sfd, result->ai_addr, result->ai_addrlen) == 0 && listen(sfd, 5) == 0)
                break;
            close(sfd);
            sfd = -1;
        }
        if (sfd == -1)
            return -2;
        return accept(sfd, NULL, NULL);
    }
}


int sans_disconnect(int socket) {
    // closes the socket
    return close(socket);
}