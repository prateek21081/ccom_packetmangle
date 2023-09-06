#include <stdio.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <string.h>

#include <netinet/ip.h> // For struct ip
#include <netinet/tcp.h> // For struct tcphdr
#include <netinet/udp.h> // For struct udphdr

unsigned long packet_count = 0;

static int callback(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data) {
    struct nfqnl_msg_packet_hdr *ph;
    ph = nfq_get_msg_packet_hdr(nfa);

    unsigned char *pktData;
    int len = nfq_get_payload(nfa, &pktData);

    struct ip *ipHeader = (struct ip *)pktData;

    // Check if it's TCP or UDP (IPv4 only for simplicity)
    if (ipHeader->ip_v == 4) {
        if (ipHeader->ip_p == IPPROTO_TCP) {
            struct tcphdr *tcpHeader = (struct tcphdr *)(pktData + ipHeader->ip_hl * 4);
            if (ntohs(tcpHeader->dest) == 27015) {
                unsigned char modified_pkt[len + 16];
                memcpy(modified_pkt, pktData, len);
                printf("tcp: ");
                for (int i = 0; i < len +  16; i++)
                    printf("%c", modified_pkt[i]);
                printf("\n");
                memset(&modified_pkt[len], 0xFF, 16);
                return nfq_set_verdict(qh, ntohl(ph->packet_id), NF_ACCEPT, len + 16, modified_pkt);
            }
        } else if (ipHeader->ip_p == IPPROTO_UDP) {
            struct udphdr *udpHeader = (struct udphdr *)(pktData + ipHeader->ip_hl * 4);
            if (ntohs(udpHeader->dest) == 27015) {
                unsigned char modified_pkt[len + 16];
                memcpy(modified_pkt, pktData, len);
                printf("udp: ");
                for (int i = 0; i < len +  16; i++)
                    printf("%c", modified_pkt[i]);
                printf("\n");
                memset(&modified_pkt[len], 0xFF, 16);
                return nfq_set_verdict(qh, ntohl(ph->packet_id), NF_ACCEPT, len + 16, modified_pkt);
            }
        }
    }

    return nfq_set_verdict(qh, ntohl(ph->packet_id), NF_ACCEPT, 0, NULL);
}

/*static int callback(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data) {
    struct nfqnl_msg_packet_hdr *ph;
    ph = nfq_get_msg_packet_hdr(nfa);

    packet_count++;
    printf("Packet Count: %lu\n", packet_count);

    unsigned char *pktData;
    int len = nfq_get_payload(nfa, &pktData);

    unsigned char modified_pkt[len + 4];
    memcpy(modified_pkt, pktData, len);
    memset(&modified_pkt[len], 0xFF, 4); // You can replace this with any 4 bytes you wish

    return nfq_set_verdict(qh, ntohl(ph->packet_id), NF_ACCEPT, len + 4, modified_pkt);
}*/

int main() {
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

