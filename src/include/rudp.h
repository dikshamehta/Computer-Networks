//rudp.h file

#define DAT 0 // 0000 0000
#define SYN 1 // 0000 0001
#define ACK 2 // 0000 0010
#define FIN 4 // 0000 0100

#define PAYLOAD_LEN 1024

typedef struct {
    char type;
    int seqnum;
    char payload[PAYLOAD_LEN];
} rudp_packet_t;

/* check
 *
 * char syn = type & SYN
 * if syn is 0 then it is not active, otherwise it is
 *
 * char fin = type & FIN
 * if fin is 0 then it is not active, otherwise it is
 *
 * set
 * type = type | SYN
 * type = type | FIN
 *
 * reset
 * type = type & ~SYN
*/
