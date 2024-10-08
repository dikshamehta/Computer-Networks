#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "include/rudp.h"

#define SWND_SIZE 10

#define MAX_TRIES 4

typedef struct {
    int socket;
    int packetlen;
    rudp_packet_t* packet;
} swnd_entry_t;

typedef struct connection_rudp_t {
    int sfd;
    struct sockaddr_in addr;
    size_t seq_num;
} connection_rudp_t;

// Global variables
swnd_entry_t send_window[SWND_SIZE];
extern connection_rudp_t connection_rudp;

static int sent = 0;
static int count = 0;
static int head = 0;
static int tail = 0;

// Function declarations
static int configure_socket_timeout(int socket, long timeout_sec, long timeout_usec);
void enqueue_packet(int sock, rudp_packet_t* pkt, int len);
static void dequeue_packet(void);
void* sans_backend(void* unused);

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



// Push a packet onto the ring buffer.
void enqueue_packet(int sock, rudp_packet_t* pkt, int len) {
    rudp_packet_t* allocated_packet = malloc(sizeof(rudp_packet_t) + len);
    if (!allocated_packet) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    allocated_packet->type = pkt->type;
    allocated_packet->seqnum = pkt->seqnum;

    memcpy(allocated_packet->payload, pkt->payload, len);

    // Wait until there is space in the buffer
    while (count - sent >= SWND_SIZE);

    swnd_entry_t new_entry;
    new_entry.socket = sock;
    new_entry.packetlen = sizeof(rudp_packet_t) + len;
    new_entry.packet = allocated_packet;

    send_window[tail] = new_entry;
    tail = (tail + 1) % SWND_SIZE;
    count++;
}

// Remove a completed packet from the ring buffer.
static void dequeue_packet(void) {
    free(send_window[head].packet);
    sent++;
    head = (head + 1) % SWND_SIZE;
}

static void transmit_packets(void) {
    int current = head;
    while (current != tail) {
        if (sendto(send_window[current].socket,send_window[current].packet,send_window[current].packetlen, 0,
                   (struct sockaddr*)&connection_rudp.addr, sizeof(connection_rudp.addr)) < 0) {
            printf("sendto failed\n");
        }

        current = (current + 1) % SWND_SIZE;
    }
}

//Resend all packets that have not been acknowledged.
static void retransmit_packets(void) {
    int current = head;
    while (current != tail) {
        if (sendto(send_window[current].socket,send_window[current].packet,send_window[current].packetlen, 0,
                   (struct sockaddr*)&connection_rudp.addr, sizeof(connection_rudp.addr)) < 0) {
            printf("sendto failed\n");
        }

        current = (current + 1) % SWND_SIZE;
    }
}

// Backend thread to handle retransmissions and acknowledgments.
void* sans_backend(void* unused_argument) {
    while (1) {
        if (count - sent > 0) {
            transmit_packets();

            configure_socket_timeout(send_window[head].socket, 0, 5000); // Set timeout to 200ms

            int retry_count = 0;
            while ((count - sent) > 0 && retry_count < MAX_TRIES ) {
                int current = head;

                int all_acknowledged = 1;
                while (current != tail) {
                    socklen_t addr_len = sizeof(connection_rudp.addr);

                    rudp_packet_t received_packet;
                    int n_recvd_bytes = recvfrom(
                            send_window[current].socket, &received_packet, sizeof(received_packet),0, (struct sockaddr*)&connection_rudp.addr, &addr_len);

                    if(n_recvd_bytes > 0 && (received_packet.type & ACK) && received_packet.seqnum == send_window[current].packet->seqnum) {
                        dequeue_packet();
                        current = (current + 1) % SWND_SIZE;
                    } else if (n_recvd_bytes < 0 ) {
                        retransmit_packets();
                        all_acknowledged = 0;
                        break;
                    }
                }

                if (all_acknowledged) {
                    break;
                }
                retry_count++;
            }

            if (retry_count == MAX_TRIES) {
                printf("Maximum retry limit reached. Some packets may not have been acknowledged.\n");
            }
        }
    }
}
