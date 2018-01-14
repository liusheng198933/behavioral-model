from scapy.all import sniff, sendp
import struct
import sys

def handle_pkt(pkt):
    pkt = str(pkt)
    preamble = pkt[:8]
    preamble_exp = "\x00" * 8
    if preamble != preamble_exp: return
    pktTMP = struct.unpack("!I", pkt[8:12])[0]
    msg = pkt[66:len(pkt)]
    print pktTMP
    print msg
    sys.stdout.flush()

def main():
    sniff(iface = "eth0",
          prn = lambda x: handle_pkt(x))

if __name__ == '__main__':
    main()
