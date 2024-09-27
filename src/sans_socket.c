//sans_socket.c file

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "include/rudp_diksha.h"
#include "include/sans.h"
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#define MAX_SOCKETS 10
#define PKT_LEN 1400

#define MAX_TRIES 5
#define HASH_TABLE_SIZE 100005
#define NEW 0
#define SYN_RECV 1
#define ACK_RECV 2
#define SYNACK_SENT 3
#define SYNACK_RECV 4
#define EST 5

struct hash_bucket {
    struct sockaddr addr;
    socklen_t addrlen;
    int status;
};

struct hash_bucket hash_table[HASH_TABLE_SIZE];

unsigned long hash(const char *buf, size_t length) {
    unsigned long hash = 5381;
    int c;
    size_t i = 0;

    while ((c = *buf++) && i < length) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        i++;
    }

    return hash % HASH_TABLE_SIZE;
}

int rudp_connect(int sfd, const struct sockaddr *addr, socklen_t addrlen) {
    printf("%d\n", __LINE__);
    struct timeval timeout = {
            .tv_usec = 20000 //in microseconds
    };
    setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    printf("%d\n", __LINE__);
    rudp_packet_t syn_packet;
    syn_packet.type = 0;
    syn_packet.seqnum = 0;
    syn_packet.type = syn_packet.type | SYN;

    printf("%d\n", __LINE__);


    //Send a packet with the SYN bit set to 1 and try for MAX_TRIES time
    int tries = 0;
    while(tries<MAX_TRIES){
        if(sendto(sfd, &syn_packet, sizeof(syn_packet), 0, addr, addrlen)==-1){
            //error
            printf("Error in sent to while syn\n");
            return -1;
        }

        rudp_packet_t syn_ack_packet;
        struct sockaddr sender_addr;
        socklen_t sender_addrlen = sizeof(sender_addr);
        if(recvfrom(sfd, &syn_ack_packet, sizeof(syn_ack_packet), 0, &sender_addr, &sender_addrlen)<0){
            // if timeout happened then continue the loop
            if(errno==ETIMEDOUT){
                tries++;
                continue;
            }
            // if some other error then break
            printf("Error in receiving synack\n");
            return -1;
        }

        // now check if in the response the SYN and ACK bits are set 1 or not
        if((syn_ack_packet.type & SYN) && (syn_ack_packet.type & ACK)){
            rudp_packet_t ack_packet;
            ack_packet.type = ACK;
            ack_packet.seqnum = 0;

            if(sendto(sfd, &ack_packet, sizeof(ack_packet), 0, addr, addrlen)==-1){
                //error
                printf("Error in sent to while ack\n");
                return -1;
            }
        }
    }
    if(tries>MAX_TRIES){
        printf("tries expired\n");
        return -1;
    }
    return 0;
}

int sans_connect(const char* host, int port, int protocol) {
    printf("sans connect\n");
    if(protocol != IPPROTO_TCP && protocol != IPPROTO_RUDP){
        return -1;
    }

    if (protocol == IPPROTO_TCP) {
        printf("abc");

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
    else{
        //RUDP
        printf("abcRUDPConnecttt\n");
        struct addrinfo *addr_info_result;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

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

            if (rudp_connect(sfd, result->ai_addr, result->ai_addrlen) == 0)
                break; // Success

            close(sfd); // Close on failure
            sfd = -1;
        }

        return sfd;

    }
}

int rudp_accept(int sfd){
    // clear hash table
    memset(hash_table, 0, HASH_TABLE_SIZE * sizeof(struct hash_bucket));
    printf("%d\n", __LINE__);

    // set timeout to 20 ms
//    struct timeval timeout = {
//            .tv_usec = 20000 //in microseconds
//    };
//    setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int synack_sent = 0;
    while(1) {
        printf("%d\n", __LINE__);

        rudp_packet_t syn_packet;
        struct sockaddr sender_addr;
        socklen_t sender_addrlen = sizeof(sender_addr);

        printf("%d\n", __LINE__);
        printf("sfc: %d\n", sfd);

        int timed_out = 0;
//        if (recvfrom(sfd, &syn_packet, sizeof(syn_packet), 0, &sender_addr, &sender_addrlen) < 0) {
        if (recvfrom(sfd, &syn_packet, sizeof(syn_packet), 0, NULL, NULL) < 0) {
            timed_out = 1;
//            if (errno == ETIMEDOUT) timed_out = 1;
//            else return -1;
        }
        printf("timeout %d\n", timed_out);
        printf("%d\n", __LINE__);


        if (timed_out && !synack_sent)
            continue;

        if (((syn_packet.type & SYN) && !(syn_packet.type & ACK)) || timed_out) {
            // send ack + syn
            rudp_packet_t synack_packet;
            memset(&synack_packet, 0, sizeof(synack_packet));
            synack_packet.type = SYN | ACK;
            sendto(sfd, &synack_packet, sizeof(syn_packet), 0, &sender_addr, sender_addrlen);
            synack_sent = 1;
        } else if (!(syn_packet.type & SYN) && (syn_packet.type & ACK)) {
            // connection established
            unsigned long key = hash((char *) &sender_addr, sender_addrlen);
            hash_table[key].status = EST;
            hash_table[key].addr = sender_addr;
            hash_table[key].addrlen = sender_addrlen;

            return sfd;
        }
    }
}

int sans_accept(const char* addr, int port, int protocol) {
    printf("sans_accept\n");
    if(protocol != IPPROTO_TCP && protocol != IPPROTO_RUDP){
        return -1;
    }
    if (protocol == IPPROTO_TCP) {
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
    else {
        printf("rudp\n");
        // RUDP
        struct addrinfo *addr_info_result;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        char port_str[6]; // Maximum of 5 digits + null terminator
        snprintf(port_str, sizeof(port_str), "%d", port);

        printf("rudp2\n");
        if (getaddrinfo(addr, port_str, &hints, &addr_info_result) != 0)
            return -2;
        printf("rudp3\n");
        struct addrinfo *result = addr_info_result;
        int sfd = -1; // socket file descriptor
        for (result = addr_info_result; result != NULL; result = result->ai_next) {
            sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
            if (sfd == -1)
                continue;
            printf("sfd\n");
            if (bind(sfd, result->ai_addr, result->ai_addrlen) == 0)
                break;

            close(sfd);
            sfd = -1;
        }
        if (sfd == -1)
            return -2;
        printf("before rudp_accept\n");
        return rudp_accept(sfd);
    }
}


int sans_disconnect(int socket) {
    if(!close(socket))
        return -1;
    return 0;
}