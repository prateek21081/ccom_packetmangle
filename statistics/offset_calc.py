#!/usr/bin/python3
from pprint import pprint
from scapy.all import *
import re
import sys


# search for Hello World in the payload
def find_hello(pkt):
    try:
        search = re.search(b"Hello World", pkt.load)
        if (search):
            # return the location of the string in the payload
            return search.start()
    except:
        pass
    return -1

# search for the string "Hello World" in the payload of the packets

unique_positions = dict()

if (len(sys.argv) != 2):
    print("Usage: ./pkt_len_freq.py <file_name>")
capture = rdpcap(sys.argv[1])
hello_world = 0
for pkt in capture:
    pos = find_hello(pkt)
    if (pos != -1):
        hello_world += 1
        unique_positions[pos] = unique_positions.get(pos, 0) + 1
        
# convert value to percentage
total = sum(unique_positions.values())
unique_positions = {k: round((v/total) * 100, 5) for k, v in unique_positions.items()}

# sort in descending order
unique_positions = list(sorted(unique_positions.items(), key=lambda x: x[1], reverse=True))

print(f"Hello World packets: {hello_world} out of {len(capture)} packets")
print("Position\tFrequency")
print('\n'.join([f"{pos}\t\t{freq}" for pos, freq in unique_positions]))