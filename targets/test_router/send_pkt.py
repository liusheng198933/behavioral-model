from scapy.all import *

class TimeStamp(Packet):
    name = "TimeStamp"
    fields_desc = [
        LongField("preamble", 0),
        IntField("pktTMP", 0),
    ]

def main(num, h_src, h_dst):
    msg = "Sheng!"
    #p = TimeStamp(pktTMP=2) / Ether(dst='00:04:00:00:00:0%s' % h_dst) / IP(src='10.0.%s.10' % h_src, dst='10.0.%s.10' % h_dst) / TCP() / msg
    p = TimeStamp(pktTMP=1) / Ether(dst='00:04:00:00:00:01') / IP(src='10.0.0.10', dst='10.0.1.10') / TCP() / msg
    hexdump(p)
    print p.show()
    for i in range(num):
        sendp(p, iface='eth0')

if __name__ == '__main__':
    main(int(sys.argv[1]), sys.argv[2], sys.argv[3])
