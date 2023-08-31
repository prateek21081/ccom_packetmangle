from netfilterqueue import NetfilterQueue
from scapy.all import *

def packet_listener(pkt):
    sc_pkt = IP(pkt.get_payload())
    if sc_pkt.dst == '192.168.226.165' and sc_pkt.haslayer("UDP"):
        # add extra bytes at end of packet
        payload = bytearray(bytes(sc_pkt))
        #payload[-1:] = b'f'
        # update sc_pkt with new payload
        sc_pkt = IP(payload)
        sc_pkt[UDP].len = len(bytes(sc_pkt[UDP]))
        sc_pkt[UDP].chksum = 0x0000
        pkt.set_payload(bytes(sc_pkt))
        # check if packet is actually modified
        modified_pkt = IP(pkt.get_payload())
        print(modified_pkt.show())
    pkt.accept()

nfqueue = NetfilterQueue()
nfqueue.bind(1, packet_listener)
try:
    nfqueue.run()
except KeyboardInterrupt:
    print('')
nfqueue.unbind()
