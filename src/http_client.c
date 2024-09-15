#include <netinet/in.h>
#include "include/sans.h"
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 1024

int http_client(const char* host, int port) {
    char request_packet[BUFFER_SIZE];
    char receive_packet[BUFFER_SIZE];
    char method[10];
    char path[256];

    scanf("%s", method);

    if (strcmp(method, "GET") != 0) {
        printf("Only GET method is supported.\n");
        return 0;
    }

    scanf("%s", path);

    // connect to server
    int socket_fd = sans_connect(host, port, IPPROTO_TCP);

    if (socket_fd < 0) {
        printf("Could not connect to server\n");
        return -1;
    }

    // creating packet to send request
    char host_header[100];
    snprintf(host_header, sizeof(host_header), "Host: %s:%d", host, port);

    // Construct the full HTTP request
    snprintf(request_packet, sizeof(request_packet),
             "%s /%s HTTP/1.1\r\n"
             "%s\r\n"
             "User-Agent: sans/1.0\r\n"
             "Cache-Control: no-cache\r\n"
             "Connection: close\r\n"
             "Accept: */*\r\n\r\n", method, path, host_header);

    // sending packet over socket
    int bytes_trans = sans_send_pkt(socket_fd, request_packet, strlen(request_packet));

    if (bytes_trans <= 0) {
        printf("Data not transmitted properly\n");
        return -1;
    }

    // Receive the response
    int bytes_rcv;
    while ((bytes_rcv = sans_recv_pkt(socket_fd, receive_packet, BUFFER_SIZE-1)) > 0) {
        receive_packet[bytes_rcv] = '\0'; //buffer should be terminated by null
        printf("%s", receive_packet);
    }

    if (bytes_rcv < 0) {
        printf("Receiving data failed\n");
    }

    //breaking the socket
    sans_disconnect(socket_fd);

    return 1;
}




