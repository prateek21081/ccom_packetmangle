from netfilterqueue import NetfilterQueue
from scapy.all import *

def packet_listener(pkt):
    sc_pkt = IP(pkt.get_payload())
    if sc_pkt.dst == '192.168.226.165' and sc_pkt.haslayer("UDP"):
        payload = bytes(sc_pkt)
        print(payload)
        pkt.set_payload(payload)
    pkt.accept()

nfqueue = NetfilterQueue()
nfqueue.bind(0, packet_listener)
try:
    nfqueue.run()
except KeyboardInterrupt:
    print('')
nfqueue.unbind()
