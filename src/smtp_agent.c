#include <stdio.h>
#include <string.h>
#include "include/sans.h"
#include <netinet/in.h>
//#include "include/sans_socket.h"

#define MAX_BUFFER_SIZE 1024

// Function to send and receive a packet
int transmit_packet(int sock_fd, char *buffer) {
    if (sans_send_pkt(sock_fd, buffer, strlen(buffer)) < 0) {
        sans_disconnect(sock_fd);
        return -1;
    }

    int received_bytes;
    if ((received_bytes = sans_recv_pkt(sock_fd, buffer, MAX_BUFFER_SIZE - 1)) <= 0) {
        sans_disconnect(sock_fd);
        return -1;
    }
    buffer[received_bytes] = '\0';  // Null-terminate the received data
    printf("%s\n", buffer);

    return 0;
}

// Function to establish connection and receive initial response
int initiate_connection(const char *server_host, int server_port, int *sock_fd, char *buffer) {
    *sock_fd = sans_connect(server_host, server_port, IPPROTO_TCP);
    if (*sock_fd < 0) {
        printf("Connection to server failed\n");
        return -1;
    }

    int received_bytes = sans_recv_pkt(*sock_fd, buffer, MAX_BUFFER_SIZE - 1);
    if (received_bytes <= 0) {
        printf("Failed to receive initial response from server\n");
        sans_disconnect(*sock_fd);
        return -1;
    }

    buffer[received_bytes] = '\0';
    printf("%s\n", buffer);
    return 0;
}

// Function to send HELO command and check the response
int send_helo_command(int sock_fd, char *buffer) {
    snprintf(buffer, MAX_BUFFER_SIZE, "HELO lunar.open.sice.indiana.edu\r\n");
    if (transmit_packet(sock_fd, buffer) < 0) {
        return -1;
    }

    if (strncmp(buffer, "250", 3) != 0) {
        printf("Error in %s command\n", "HELO");
        return -1;
    }

    return 0;
}

// Function to send MAIL FROM and RCPT TO commands
int send_mail_and_recipient(int sock_fd, char *buffer, const char *email) {
    snprintf(buffer, MAX_BUFFER_SIZE, "MAIL FROM: <%s>\r\n", email);
    if (transmit_packet(sock_fd, buffer) < 0) {
        return -1;
    }

    snprintf(buffer, MAX_BUFFER_SIZE, "RCPT TO: <%s>\r\n", email);
    if (transmit_packet(sock_fd, buffer) < 0) {
        return -1;
    }
    return 0;
}

// Function to send DATA command and email content
int send_email_content(int sock_fd, char *buffer, const char *email_file_path) {
    snprintf(buffer, MAX_BUFFER_SIZE, "DATA\r\n");
    if (transmit_packet(sock_fd, buffer) < 0) {
        return -1;
    }

    FILE *email_file = fopen(email_file_path, "r");
    if (email_file == NULL) {
        perror("Error opening file");
        return -1;
    }

    // Read email content and send it to the server
    while (fgets(buffer, MAX_BUFFER_SIZE, email_file) != NULL) {
        if (sans_send_pkt(sock_fd, buffer, strlen(buffer)) < 0) {
            fclose(email_file);
            sans_disconnect(sock_fd);
            return -1;
        }
    }
    fclose(email_file);

    // Terminate the email content
    snprintf(buffer, MAX_BUFFER_SIZE, "\r\n.\r\n");
    if (transmit_packet(sock_fd, buffer) < 0) {
        return -1;
    }
    return 0;
}

// SMTP Agent main function
int smtp_agent(const char *smtp_host, int smtp_port) {
    char sender_email[256];
    char email_path[256];
    scanf("%s", sender_email);
    scanf("%s", email_path);


    char buffer[MAX_BUFFER_SIZE];
    int sock_fd;
    if (initiate_connection(smtp_host, smtp_port, &sock_fd, buffer) < 0) {
        return -1;
    }

    if (send_helo_command(sock_fd, buffer) < 0) {
        return -1;
    }

    if (send_mail_and_recipient(sock_fd, buffer, sender_email) < 0) {
        return -1;
    }

    if (send_email_content(sock_fd, buffer, email_path) < 0) {
        return -1;
    }

    snprintf(buffer, MAX_BUFFER_SIZE, "QUIT\r\n");
    if (transmit_packet(sock_fd, buffer) < 0) {
        return -1;
    }

    sans_disconnect(sock_fd);

    return 0;
}




//#include <stdio.h>
//#include <string.h>
//#include "include/sans.h"
//#include <netinet/in.h>
//
//#define BUFFER_SIZE 1024
//
//int handle_error(char *request_buffer){
//
//    if (strncmp(request_buffer, "250", 3) != 0) {
//        printf("issue with HELO/");
//    }
//    return 0;
//}
//
//int send_receive_packet(int socket_fd, char *request_buffer){
//    if (sans_send_pkt(socket_fd, request_buffer, strlen(request_buffer)) < 0) {
//        sans_disconnect(socket_fd);
//        return -1;
//    }
//
//    int bytes_rcv;
//
//    if((bytes_rcv = sans_recv_pkt(socket_fd,  request_buffer, BUFFER_SIZE-1))<=0){
//        sans_disconnect(socket_fd);
//        return -1;
//    }
//    request_buffer[bytes_rcv] = '\0';
//    printf("%s\n", request_buffer);
//
//    return 0;
//}
//
//int smtp_agent(const char* host, int port) {
//    char request_buffer[BUFFER_SIZE];
//    char email_address[256];
//    char file_path[256];
//    int bytes_rcv;
//
//    // Taking input
//    scanf("%s", email_address);
//    scanf("%s", file_path);
//
//    //Establish connection
//    int socket_fd = sans_connect(host, port, IPPROTO_TCP);
//
//    // Receiving initial response
//    if((bytes_rcv = sans_recv_pkt(socket_fd,  request_buffer, BUFFER_SIZE-1))<=0){
//        printf("Request not received properly");
//        sans_disconnect(socket_fd);
//        return -1;
//    }
//    request_buffer[bytes_rcv] = '\0';
//    printf("%s\n", request_buffer);
//
//    // Sending a HELO message
//    snprintf(request_buffer, sizeof(request_buffer),
//             "HELO lunar.open.sice.indiana.edu\r\n");
//
//    if(send_receive_packet(socket_fd, request_buffer)<0){
//        return -1;
//    }
//    //ensuring HELO response contains 250 code
//    if(handle_error(request_buffer) < 0){
//        sans_disconnect(socket_fd);
//        return -1;
//    }
//
//    // sending a MAIL FROM message
//    snprintf(request_buffer, sizeof(request_buffer), "MAIL FROM: <%s>\r\n", email_address);
//    if(send_receive_packet(socket_fd, request_buffer)<0){
//        return -1;
//    }
//
//    // sending a RCPT TO message
//    snprintf(request_buffer, sizeof(request_buffer), "RCPT TO: <%s>\r\n", email_address);
//    if(send_receive_packet(socket_fd, request_buffer)<0){
//        return -1;
//    }
//
//    // sending a DATA message
//    snprintf(request_buffer, sizeof(request_buffer), "DATA\r\n");
//    if(send_receive_packet(socket_fd, request_buffer)<0){
//        return -1;
//    }
//
//    // Reading email content
//    FILE *email_file;
//    email_file = fopen(file_path, "r");
//    if (email_file == NULL) {
//        perror("fopen");
//    }
//    while (fgets(request_buffer, sizeof(request_buffer), email_file) != NULL) {
//        bytes_rcv = sans_send_pkt(socket_fd, request_buffer, strlen(request_buffer));
//        if (bytes_rcv < 0) {
//            fclose(email_file);
//            sans_disconnect(socket_fd);
//            return -1;
//        }
//    }
//    fclose(email_file);
//
//    // Sending termination condition
//    snprintf(request_buffer, sizeof(request_buffer), "\r\n.\r\n");
//    if(send_receive_packet(socket_fd, request_buffer)<0){
//        return -1;
//    }
//
//    // sending QUIT message
//    snprintf(request_buffer, sizeof(request_buffer), "QUIT\r\n");
//    if(send_receive_packet(socket_fd, request_buffer)<0){
//        return -1;
//    }
//
//    // disconnecting the socket
//    sans_disconnect(socket_fd);
//
//    return 0;
//}
