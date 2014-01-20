#include "tcp.h"
#include "hw.h"
#include "net.h"

#define FIN_FLAG 1 << 0
#define SYN_FLAG 1 << 1
#define RST_FLAG 1 << 2
#define PSH_FLAG 1 << 3
#define ACK_FLAG 1 << 4

#define WORD_SIZE 4
#define TCP_MIN_HEADER_SIZE 5

#define TCP_MAX_SEGEMENT_SIZE 1000

#define TCP_STATE_LISTEN 1
#define TCP_STATE_SYN_SENT 2
#define TCP_STATE_SYN_RECEIVED 3
#define TCP_STATE_ESTABLISHED 4

typedef union PACKED tcp_header {
    struct PACKED {
        uint16_t src_port;
        uint16_t dest_port;
        uint32_t seq_num;
        uint32_t ack_num;
        uint8_t data_offset; // 4-bits - bata offset (x 32-bit words), 3-bit - reserved, 1-bit NS flag
        uint8_t flags;       // CWR, ECE, URG, ACK, PSH, RST, SYN, FIN  
        uint16_t window_size;
        uint16_t checksum;
        uint16_t urg_pointer; // See URG flag
        uint32_t options[10]; // Maximum of 320 bits, divisible by 32
    };
    uint8_t buffer[TCP_MAX_SEGEMENT_SIZE];
    
} tcp_header_t;

/* DELETE ME
SYN: The active open is performed by the client sending a SYN to the server. The client sets the segment's sequence number to a random value A.
SYN-ACK: In response, the server replies with a SYN-ACK. The acknowledgment number is set to one more than the received sequence number i.e. A+1, and the sequence number that the server chooses for the packet is another random number, B.
ACK: Finally, the client sends an ACK back to the server. The sequence number is set to the received acknowledgement value i.e. A+1, and the acknowledgement number is set to one more than the received sequence number i.e. B+1.
*/
size_t tcp_connect(session_t *session, const uint8_t dst_ip[], uint16_t dst_port)
{
    tcp_header_t hdr, rcv;
 
    // SYN
    hdr.src_port = netb_s(dst_port);
    hdr.dest_port = netb_s(dst_port);
    hdr.ack_num  = hdr.seq_num = 0; /// @todo: this should be random
    hdr.data_offset = TCP_MIN_HEADER_SIZE << 4;
    hdr.flags = SYN_FLAG;
    hdr.window_size = netb_l(1000);
    hdr.urg_pointer = 0;
    hdr.checksum = 0;

    hdr.checksum = ip_chksum(session, dst_ip, IP_PROTOCOL_TCP, hdr.buffer, TCP_MIN_HEADER_SIZE * WORD_SIZE);
    int res = ip_send(session, dst_ip, IP_PROTOCOL_TCP, hdr.buffer, TCP_MIN_HEADER_SIZE * WORD_SIZE);
    if(res <= 0) {
        return res;
    }

    session->tcp_state = TCP_STATE_SYN_SENT;

    // SYN-ACK
    while((res = ip_recv(session, rcv.buffer, TCP_MIN_HEADER_SIZE * WORD_SIZE)) > 0)
    {
        if(!(rcv.flags & ACK_FLAG) || !(rcv.flags & SYN_FLAG) 
            || rcv.ack_num != netb_l(hostb_l(hdr.seq_num) + 1)
            || rcv.dest_port != netb_s(dst_port)) 
        {
            continue;
        }
    }
    
    if(res <= 0)
    {
        return res;
    }

    session->tcp_seq = hostb_l(rcv.ack_num);

    // ACK
    hdr.seq_num = rcv.ack_num;
    hdr.ack_num = netb_l(hostb_l(rcv.seq_num) + 1);
    hdr.flags = ACK_FLAG;
    hdr.checksum = 0;
    hdr.checksum = ip_chksum(session, dst_ip, IP_PROTOCOL_TCP, hdr.buffer, 20);

    res = ip_send(session, dst_ip, IP_PROTOCOL_TCP, hdr.buffer, (hdr.data_offset >> 4) * 4);
    if(res <= 0) {
        return res;
    }

    session->tcp_ack = hostb_l(hdr.ack_num);
    session->tcp_state = TCP_STATE_ESTABLISHED;

    return 1;
}

session_t *tcp_listen(session_t *session, const uint8_t bind_ip[], uint16_t bind_port)
{
    int res;
    tcp_header_t hdr, rcv;

    session->tcp_state = TCP_STATE_LISTEN;

    // SYN
    while((res = ip_recv(session, rcv.buffer, 20)) > 0)
    {
        if(!(rcv.flags & SYN_FLAG) || (rcv.flags & ACK_FLAG) || rcv.dest_port != netb_s(bind_port)) 
        {
            continue;
        }
    }

    if(res <= 0)
        return 0;

    // SYN-ACK
    hdr.src_port = netb_s(bind_port);
    hdr.dest_port = netb_s(bind_port);
    hdr.seq_num = rcv.seq_num; /// @todo: this should be random
    hdr.ack_num = netb_l(hostb_l(rcv.seq_num) + 1);
    hdr.data_offset = 5 << 4;
    hdr.flags = SYN_FLAG | ACK_FLAG;
    hdr.window_size = netb_l(1000);
    hdr.urg_pointer = 0;
    hdr.checksum = 0;
    hdr.checksum = ip_chksum(session, bind_ip, IP_PROTOCOL_TCP, hdr.buffer, 20);

    res = ip_send(session, bind_ip, IP_PROTOCOL_TCP, hdr.buffer, (hdr.data_offset >> 4) * 4);
    if(res <= 0) {
        return 0;
    }

    // ACK
    while((res = ip_recv(session, rcv.buffer, 20)) > 0)
    {
        if((rcv.flags & SYN_FLAG) || !(rcv.flags & ACK_FLAG) 
            || rcv.dest_port != netb_s(bind_port)
            || rcv.ack_num != netb_l(hostb_l(hdr.seq_num) + 1)
            || rcv.seq_num != hdr.ack_num) 
        {
            continue;
        }
    }
    if(res <= 0) {
        return 0;
    }

    session_t *nsess = net_init(session->interface, bind_port, session->protocol);
    nsess->tcp_state = TCP_STATE_ESTABLISHED;
    nsess->tcp_ack = hostb_l(rcv.seq_num) + 1;
    nsess->tcp_seq = hostb_l(hdr.seq_num);

    return nsess;
}

size_t tcp_send(session_t *session, const uint8_t data[], size_t data_len)
{
    return 0;
}

size_t tcp_recv(session_t *session, uint8_t buffer[], size_t buffer_len)
{
    return 0;
}

size_t tcp_close(session_t *session)
{
    return 0;
}