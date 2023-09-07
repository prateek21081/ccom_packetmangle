import pyshark
import statistics
from datetime import datetime
# Open the pcapng file for reading
cap = pyshark.FileCapture('latency_cap_server.pcapng' )

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
            diffs.append((obj - sniff_time).total_seconds())

diffs.sort()

print("Mean: ", statistics.mean(diffs))
print("Median: ", statistics.median(diffs))
print("Standard Deviation: ", statistics.stdev(diffs))
print("Variance: ", statistics.variance(diffs))
print("Max: ", diffs[-1])
print("Min: ", diffs[0])