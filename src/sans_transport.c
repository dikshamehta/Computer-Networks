#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include "include/sans.h"
#include "include/rudp.h"
#include <stdio.h>

typedef struct connection_rudp_t {
    int sfd;
    struct sockaddr_in addr;
    size_t seq_num;
} connection_rudp_t;

extern connection_rudp_t connection_rudp;

void enqueue_packet(int sock, rudp_packet_t* pkt, int len);

//handling Timeout
static int configure_socket_timeout(int socket, long timeout_sec, long timeout_usec) {
    struct timeval timeout = {
            .tv_sec = timeout_sec,
            .tv_usec = timeout_usec
    };
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        printf("setsockopt failed\n");
        return -1;
    }
    return 0;
}


static int helper_send_packet(int socket, rudp_packet_t *packet) {
    if (sendto(socket, packet, sizeof(*packet), 0,
               (struct sockaddr*)&connection_rudp.addr, sizeof(connection_rudp.addr)) < 0) {
        printf("sendto failed\n");
        return -1;
    }
    return 0;
}

static int helper_receive_packet(int socket, rudp_packet_t *packet) {
    socklen_t addr_len = sizeof(connection_rudp.addr);
    int bytes_received = recvfrom(socket, packet, sizeof(*packet), 0,
                                  (struct sockaddr*)&connection_rudp.addr, &addr_len);
    return bytes_received;
}

int sans_send_pkt(int socket, const char* buf, int len) {
    if (buf == NULL || len <= 0) return -1;

    if (configure_socket_timeout(socket, 0, 200000) < 0) {
        return -1;
    }

    // Generating Data Packet
    rudp_packet_t data_packet = { .type = DAT };

    // calculating total number of packets which can be sent full
    size_t total_full_packets = len / SIZE_OF_PAYLOAD;

    for (int i = 0; i <= total_full_packets; ++i) {
        // calculating the offset to find the memory location of data
        size_t buffer_offset = i * SIZE_OF_PAYLOAD;
        size_t size_of_payload = SIZE_OF_PAYLOAD;

        size_t curr_seq_num = connection_rudp.seq_num + i;
        data_packet.seqnum = curr_seq_num;

        //handlign the tail packet which might not be full
        if (i == total_full_packets) {
            if (len % SIZE_OF_PAYLOAD == 0) break; // If all the packet were of full size
            size_of_payload = len % SIZE_OF_PAYLOAD;  // tail packet which might not be full
        }
        memcpy(data_packet.payload, buf + buffer_offset, size_of_payload);

//        rudp_packet_t ack_pkt;
        enqueue_packet(socket, &data_packet, sizeof(data_packet));
        connection_rudp.seq_num +=1;
    }

    return len;
}

// Function to receive data
int sans_recv_pkt(int socket, char* buf, int len) {
    if (buf == NULL || len <= 0) return -1;

    // Generating recieve Packet and acknolowdgement
    rudp_packet_t packet_rcv;
    rudp_packet_t packet_ack = { .type = ACK };

    // calculating total number of packets which can be sent full
    size_t total_full_packets = len / SIZE_OF_PAYLOAD;


    for (int i = 0; i <= total_full_packets; ++i) {
        // calculating the offset to find the memory location of data
        size_t buffer_offset = i * SIZE_OF_PAYLOAD;
        size_t size_of_payload = SIZE_OF_PAYLOAD;

        size_t real_seq_num = connection_rudp.seq_num + i;

        //handlign the tail packet which might not be full
        if (i == total_full_packets) {
            if (len % SIZE_OF_PAYLOAD == 0) break; // If all the packet were of full size
            size_of_payload = len % SIZE_OF_PAYLOAD;  // tail packet which might not be full
        }

        while (1) {
            int n_recvd_bytes = helper_receive_packet(socket, &packet_rcv);
            if (n_recvd_bytes > 0 && packet_rcv.type == DAT) {
                if (packet_rcv.seqnum == real_seq_num) {
                    memcpy(buf + buffer_offset, packet_rcv.payload, size_of_payload);

                    packet_ack.seqnum = real_seq_num;
                    if (helper_send_packet(socket, &packet_ack) < 0) {
                        return -1;
                    }

                    connection_rudp.seq_num = real_seq_num + 1;
                    break;
                }

            }
        }
    }

    return len;
}