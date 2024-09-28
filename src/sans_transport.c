#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>

int sans_send_pkt(int socket, const char *buf, int len)
{
    printf("sans_send_pkt\n");
    if (buf == NULL || len <= 0)
    {
        errno = EINVAL;
        return -1;
    }

    ssize_t bytes_sent = sendto(socket, buf, len, 0, NULL, 0);

    if (bytes_sent < 0)
    {
        // Error occurred, errno is set by sendto
        return -1;
    }

    return (int)bytes_sent;
}

int sans_recv_pkt(int socket, char *buf, int len)
{
    printf("sans_recv_pkt\n");

    if (buf == NULL || len <= 0)
    {
        errno = EINVAL;
        return -1;
    }

    ssize_t bytes_received = recvfrom(socket, buf, len, 0, NULL, NULL);

    if (bytes_received < 0)
    {
        // Error occurred, errno is set by recvfrom
        return -1;
    }

    return (int)bytes_received;
}