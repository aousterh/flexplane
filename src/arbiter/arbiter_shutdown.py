'''
Created on January 8, 2014

@author: aousterh

Constructs and sends a kill packet to the arbiter. The arbiter should exit upon
receipt of this packet.

To run on an endpoint: sudo python arbiter_shutdown.py
'''

import struct
from socket import *

class arbiter_shutdown(object):
    __FASTPASS_KILL = 225

    def __init__(self):
        print 'created kill packet'

    def send_kill(self):
        ARBITER_ADDR = 0xC0A80101  # 192.168.1.1
        s = socket(AF_INET, SOCK_RAW, self.__FASTPASS_KILL)
        s.setsockopt(IPPROTO_IP, IP_HDRINCL, 0)
        dst_ip = inet_ntoa(struct.pack("!L", ARBITER_ADDR))
        s.sendto("", (dst_ip, 1))
        print 'sent kill packet'

def main():
    controller = arbiter_shutdown()
    controller.send_kill()

if __name__ =='__main__':main()
