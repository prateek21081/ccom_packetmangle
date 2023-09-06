import pyshark

# Open the pcapng file for reading
cap = pyshark.FileCapture('latency_cap_server.pcapng' )

# Loop through the packets in the file
for pkt in cap:
    # Print the packet details
    if 'ip' in pkt and 'eth' in pkt:
        pkt_bytes = (pkt.eth.trailer)
        sniff_time = pkt.sniff_time
        print(pkt_bytes, sniff_time)