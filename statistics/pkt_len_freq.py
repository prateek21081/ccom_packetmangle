#!/usr/bin/python3
import sys
from scapy.all import *
import matplotlib.pyplot as plt

if (len(sys.argv) != 2):
    print("Usage: ./pkt_len_freq.py <file_name>")
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

num_pkts = sum([x[1] for x in all_pkts_lens])

# turning frequencies into percentages
for key in pkt_len_freq:
    pkt_len_freq[key] = (pkt_len_freq[key] / num_pkts) * 100

# remove packets with with less than 0.01% frequency
pkt_len_freq = {k: v for k, v in pkt_len_freq.items() if v > 0.01}

x = pkt_len_freq.keys()
y = pkt_len_freq.values()

print(f"Malformed packets: {malformed}")
plt.bar(x, y) 
# plt.savefig(sys.argv[1].split('.')[0] + ".png")
plt.show()
