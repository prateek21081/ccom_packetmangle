from netfilterqueue import NetfilterQueue
from scapy.all import *

def packet_mangler(pkt):
    sc_pkt = IP(pkt.get_payload())
    if not sc_pkt.haslayer("UDP"):
        pkt.accept()
        return 
    if sc_pkt.dst == '192.168.226.165':
        # add extra bytes at end of packet
        payload = bytearray(bytes(sc_pkt))
        payload += b'ffffffff'
        # update sc_pkt with new payload
        sc_pkt = IP(payload)
        # sc_pkt[UDP].len = len(bytes(sc_pkt[UDP]))
        sc_pkt[UDP].chksum = 0x0000
        pkt.set_payload(bytes(sc_pkt))
        # check if packet is actually modified
        modified_pkt = IP(pkt.get_payload())
    if sc_pkt.dst == '192.168.226.176':
        # fetch extra bytes at end of packet
        payload = bytearray(bytes(sc_pkt))
        data = payload[-8:]
        print(data)
    pkt.accept()

nfqueue = NetfilterQueue()
nfqueue.bind(1, packet_mangler)
try:
    nfqueue.run()
except KeyboardInterrupt:
    print('')

nfqueue.unbind()
