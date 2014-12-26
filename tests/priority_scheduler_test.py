#!/usr/bin/python

from fastemu import *
import unittest

class TestPriorityScheuler(unittest.TestCase):
    def __init__(self, *args):
        super(TestPriorityScheuler,self).__init__(*args)
        # allocate emulation state
        self.state = emu_state()
        ADMITTED_MEMPOOL_SIZE = 1 << 10
        ADMITTED_RING_SIZE = 1 << 10
        PACKET_MEMPOOL_SIZE = 1 << 14
        PACKET_RING_SIZE = 1 << 10
        
        emu_alloc_init(self.state, ADMITTED_MEMPOOL_SIZE, ADMITTED_RING_SIZE,
                       PACKET_MEMPOOL_SIZE, PACKET_RING_SIZE)

    
    def setUp(self):
        self.bank = PacketQueueBank(4, 64, 128)
        self.prio = PriorityScheduler(self.bank)
    
    def test_001_orders_packets_within_a_port(self):
        
        for i in xrange(64):
            # enqueue packet with id 'i' into queue 'i' of port 0
            b = create_packet(self.state,0,0,0,i)
            self.bank.enqueue(0,i,b)
        
        # schedule packets, they should be scheduled in order of increasing id
        for i in xrange(64):
            pkt_p = self.prio.schedule(0)
            pkt = pkt_value(pkt_p)
            
            self.assertEqual(pkt.id, i, "packet id is %d, expected %d" % (pkt.id, i))
            
            free_packet(self.state,pkt_p)

    def test_001_keeps_order_and_handles_queue_backlog(self):
        
        for j in xrange(32):
            for i in xrange(64):
                # enqueue packet with id 'i', flow 'j' into queue 'i' of port 0
                b = create_packet(self.state,0,0,j,i)
                self.bank.enqueue(0,i,b)
        
        # schedule packets, they should be scheduled in order of increasing id
        for i in xrange(64):
            for j in xrange(32):
                pkt_p = self.prio.schedule(0)
                pkt = pkt_value(pkt_p)
                
                self.assertEqual((pkt.flow,pkt.id), (j,i), "packet (flow,id) is (%d,%d), expected (%d,%d)" % (pkt.flow,pkt.id,j,i))
                
                free_packet(self.state,pkt_p)


if __name__ == '__main__':
    unittest.main()
    