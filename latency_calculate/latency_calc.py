import pyshark
import statistics
import sys
from datetime import datetime
# Open the pcapng file for reading

if len(sys.argv) < 2:
    print("Please enter the name of the pcap file as an argument")
    exit(1)

cap = pyshark.FileCapture(sys.argv[1])

diffs = []

# Loop through the packets in the file
for pkt in cap:
    # Print the packet details
    if 'ip' in pkt and 'eth' in pkt:
        pkt_bytes = (pkt.eth.trailer)
        sniff_time = pkt.sniff_time
        pkt_bytes = pkt_bytes.split(':')
        if pkt_bytes:
            pkt_bytes[:4] = pkt_bytes[:4][::-1]
            pkt_bytes[8:] = pkt_bytes[8:][::-1]
            seconds = int("".join(pkt_bytes[:4]), 16)
            nanoseconds = int("".join(pkt_bytes[8:]), 16)
            obj = datetime.fromtimestamp(seconds + nanoseconds / 1e9)
            diffs.append(abs((obj - sniff_time).total_seconds()))

diffs.sort()

print("Mean: ", statistics.mean(diffs))
print("Median: ", statistics.median(diffs))
print("Standard Deviation: ", statistics.stdev(diffs))
print("Variance: ", statistics.variance(diffs))
print("Max: ", diffs[-1])
print("Min: ", diffs[0])