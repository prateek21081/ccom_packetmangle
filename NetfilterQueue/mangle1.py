from netfilterqueue import NetfilterQueue
from scapy.all import *

def packet_mangler(pkt):
    sc_pkt = IP(pkt.get_payload())
    payload = bytearray(bytes(sc_pkt))

    if sc_pkt.haslayer("UDP"):
        if sc_pkt.dst == '192.168.226.165':
            payload = payload + b'ffffffff'
        elif sc_pkt.dst == '192.168.226.176':
            print("data received: " + payload[-8:])
            # payload = payload[:-8]

    sc_pkt = IP(payload)
    sc_pkt[IP].len = len(bytes(sc_pkt[IP]))
    sc_pkt[UDP].len = len(bytes(sc_pkt[UDP]))
    sc_pkt[UDP].chksum = 0x0000
    pkt.set_payload(bytes(sc_pkt))
    modified_pkt = IP(pkt.get_payload())
    print(modified_pkt.show())

    pkt.accept()

nfqueue = NetfilterQueue()
nfqueue.bind(1, packet_mangler)
try:
    nfqueue.run()
except KeyboardInterrupt:
    print('')

nfqueue.unbind()
