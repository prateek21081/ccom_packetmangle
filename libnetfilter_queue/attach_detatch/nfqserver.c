#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

//default values for fallback
int PAYLOAD_LEN = 32;
int QUEUE_NUM = 0;
int FIFO_FD = 0;
char* FIFO_PATH;

static int callback(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data) {
    struct nfqnl_msg_packet_hdr *ph;
    unsigned char *pkt_data;
    struct ip *ip_hdr;
    struct udphdr *udp_hdr;
    int pkt_len;

    ph = nfq_get_msg_packet_hdr(nfa);
    pkt_len = nfq_get_payload(nfa, &pkt_data);
    ip_hdr = (struct ip *)pkt_data;
    udp_hdr = (struct udphdr *)(pkt_data + ip_hdr->ip_hl * 4);

    short num_bytes;
    unsigned char payload[PAYLOAD_LEN];

    memcpy(&num_bytes, pkt_data + pkt_len - sizeof(short), sizeof(short));
    memcpy(payload, pkt_data + pkt_len - num_bytes - sizeof(short), num_bytes);
    write(FIFO_FD, payload, num_bytes);
    printf("bytes write: %d\n", (int)num_bytes);

    return nfq_set_verdict(qh, ntohl(ph->packet_id), NF_ACCEPT, pkt_len, pkt_data);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <fifo_path> <queue_num> <payload_len>", argv[0]);
        exit(EXIT_FAILURE);
    }
    FIFO_PATH = argv[1];
    QUEUE_NUM = atoi(argv[2]);
    PAYLOAD_LEN = atoi(argv[3]);
    FIFO_FD = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    printf("info: FIFO_PATH=%s\n", FIFO_PATH);
    printf("info: QUEUE_NUM=%d\n", QUEUE_NUM);
    printf("info: PAYLOAD_LEN=%d\n", PAYLOAD_LEN);
    printf("info: FIFO_FD=%d\n", FIFO_FD);
    if (FIFO_FD == -1) {
        fprintf(stderr, "error: FIFO_FD=%d\n", FIFO_FD);
        exit(EXIT_FAILURE);
    }

    struct nfq_handle *h;
    struct nfq_q_handle *qh;

    h = nfq_open();
    if (!h) {
        perror("Error in nfq_open()");
        exit(1);
    }

    if (nfq_unbind_pf(h, AF_INET) < 0) {
        perror("Error in nfq_unbind_pf()");
        exit(1);
    }

    if (nfq_bind_pf(h, AF_INET) < 0) {
        perror("Error in nfq_bind_pf()");
        exit(1);
    }

    qh = nfq_create_queue(h, QUEUE_NUM, &callback, NULL);
    if (!qh) {
        perror("Error in nfq_create_queue()");
        exit(1);
    }

    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
        perror("Error in nfq_set_mode()");
        exit(1);
    }

    int fd = nfq_fd(h);
    char buf[4096];
    int rv;

    while ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) 
    {
        nfq_handle_packet(h, buf, rv);
    }

    nfq_destroy_queue(qh);
    nfq_close(h);

    return 0;
}
