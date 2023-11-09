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

#define PAYLOAD_LEN 16
int fifo = 1;

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

    unsigned char payload[PAYLOAD_LEN];

    memcpy(payload, pkt_data + pkt_len - PAYLOAD_LEN, PAYLOAD_LEN);
    write(fifo, payload, PAYLOAD_LEN);

    return nfq_set_verdict(qh, ntohl(ph->packet_id), NF_ACCEPT, pkt_len, pkt_data);
}

int main() {
    mkfifo("/tmp/x", 0666);
    fifo = open("/tmp/x", O_WRONLY);
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

    qh = nfq_create_queue(h, 0, &callback, NULL);
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

