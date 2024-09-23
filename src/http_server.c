#include <netinet/in.h>
#include "include/sans.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_BUFFER 1024

void send_http_error(int sockfd, const char* status) {
    char header_buf[MAX_BUFFER];
    snprintf(header_buf, sizeof(header_buf),
             "HTTP/1.1 %s\r\n"
             "Content-Length: 0\r\n"
             "Content-Type: text/html; charset=utf-8\r\n\r\n", status);
    sans_send_pkt(sockfd, header_buf, strlen(header_buf));
}

int send_file_response(int sockfd, const char* filename) {
    FILE* filestream = fopen(filename, "rb");
    char header_buf[MAX_BUFFER], body_buf[MAX_BUFFER];

    if (filestream == NULL) {
        send_http_error(sockfd, "404 Not Found");
        return -1;
    }

    struct stat file_info;
    if (stat(filename, &file_info) != 0) {
        fclose(filestream);
        return -1;
    }

    snprintf(header_buf, sizeof(header_buf),
             "HTTP/1.1 200 OK\r\n"
             "Content-Length: %ld\r\n"
             "Content-Type: text/html; charset=utf-8\r\n\r\n", file_info.st_size);
    sans_send_pkt(sockfd, header_buf, strlen(header_buf));

    size_t bytes_read;
    while ((bytes_read = fread(body_buf, 1, MAX_BUFFER, filestream)) > 0) {
        sans_send_pkt(sockfd, body_buf, bytes_read);
    }

    fclose(filestream);
    return 0;
}

int http_server(const char* interface, int port_number) {
    char recv_buf[MAX_BUFFER], method[10], path[256];
    int sockfd = sans_accept(interface, port_number, IPPROTO_TCP);
    if (sockfd < 0) {
        return -1;
    }

    int bytes_received = sans_recv_pkt(sockfd, recv_buf, MAX_BUFFER - 1);
    if (bytes_received <= 0) {
        sans_disconnect(sockfd);
        return -1;
    }
    recv_buf[bytes_received] = '\0';

    sscanf(recv_buf, "%s %s", method, path);
    char* filename = path[0] == '/' ? path + 1 : path;

    if (send_file_response(sockfd, filename) < 0) {
        sans_disconnect(sockfd);
        return 1;
    }

    sans_disconnect(sockfd);
    return 0;
}


// #include <netinet/in.h>
// #include "include/sans.h"
// #include <stdio.h>
// #include <string.h>
// #include <sys/stat.h>


// #define BUFFER_SIZE 1024

// int http_server(const char* iface, int port){
//     char receive_packet[BUFFER_SIZE];
//     int bytes_rcv;
//     int socket_fd = sans_accept(iface, port, IPPROTO_TCP);

//     if (socket_fd < 0) {
//         printf("Failed to accept connection\n");
//         return -1;
//     }

//     if((bytes_rcv = sans_recv_pkt(socket_fd,  receive_packet, BUFFER_SIZE-1))<=0){
//         printf("Request not received properly");
//     }

//     receive_packet[bytes_rcv] = '\0';

//     char method[10], path[256];

//     sscanf(receive_packet, "%s %s", method, path);

//     char *requested_filename;
//     if(path[0]=='/'){
//         requested_filename = path + 1;
//     }
//     else{
//         requested_filename = path;
//     }

//     FILE *requested_file;

//     requested_file = fopen(requested_filename, "rb");

//     char response_packet[BUFFER_SIZE];
//     char response_header[BUFFER_SIZE];

//     if (requested_file == NULL) {
//         perror("fopen");

//         snprintf(response_header, sizeof(response_header),
//                  "HTTP/1.1\r\n"
//                  "404 File not Found\r\n"
//                  "Content-Length: 0\r\n"
//                  "Content-Type: text/html; charset=utf-8 \r\n\r\n");
//         sans_send_pkt(socket_fd, response_header, strlen(response_header));
//         return 1;
//     }

//     struct stat st;

//     if (stat(requested_filename, &st) != 0) {
//         perror("stat");
//         return -1;
//     }

//     snprintf(response_header, sizeof(response_header),
//              "HTTP/1.1 200 OK\r\n"
//              "Content-Length: %ld\r\n"
//              "Content-Type: text/html; charset=utf-8 \r\n\r\n", st.st_size);

//     sans_send_pkt(socket_fd, response_header, strlen(response_header));

//     int bytes_read_file;
//     while ((bytes_read_file = fread(response_packet, 1, BUFFER_SIZE, requested_file)) > 0) {
//         sans_send_pkt(socket_fd, response_packet, bytes_read_file);
//     }

//     fclose(requested_file);

//     sans_disconnect(socket_fd);

//     return 0;
// }
