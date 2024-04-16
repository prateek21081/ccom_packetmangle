#!/usr/bin/python3
import sys
from scapy.all import *
import matplotlib.pyplot as plt

if (len(sys.argv) != 2):
    print("Usage: ./main.py <file_name>")
capture = rdpcap(sys.argv[1])

pkt_len_freq = dict()
malformed = 0

for pkt in capture:
    try:
        if (pkt.len-28 not in pkt_len_freq):
            pkt_len_freq[pkt.len-28] = 0
        pkt_len_freq[pkt.len-28] += 1
    except:
        malformed += 1


all_pkts_lens = pkt_len_freq.items()

filtered_pkts = []

for pkt in all_pkts_lens:
    if pkt[1] >= 100:
        filtered_pkts.append(pkt)

x = [pkt[0] for pkt in filtered_pkts]
y = [pkt[1] for pkt in filtered_pkts] 

print(f"Malformed packets: {malformed}")
plt.bar(x, y) 
plt.savefig(sys.argv[1].split('.')[0] + ".png")
