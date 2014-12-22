#!/usr/bin/python

from fastemu import *

class PyTorClassifier(PyClassifier):
    def classify(self, pkt_p):
        """
        classify returns a pair (port, queue): the packet's egress port, and
          the per-port queue in which the packet should be queued.
        """
        # need to convert from (struct emu_packet *) to (struct emu_packet)
        pkt = pkt_value(pkt_p)
        port = pkt.dst
        queue = pkt.flow
        return port, queue 

class PyDropQueueManager(PyQueueManager):
    def __init__(self, queue_bank, queue_capacity):
        # initialize the base class
        super(DropQueueManager, self).__init__()
        self.bank = queue_bank
        self.capacity = queue_capacity
        
    def enqueue(self, pkt_p, port, queue):
        if self.bank.occupancy(port, queue) >= self.capacity:
            drop_packet(pkt_p)
        else:
            self.bank.enqueue(port, queue, pkt_p)

class PyScheduler(PyScheduler):
    def __init__(self, queue_bank, n_queues_per_port):
        super(TestScheduler, self).__init__()
        self.bank = queue_bank
        self.n_queues_per_port = n_queues_per_port
        
    def schedule (self, port):
        for i in range(self.n_queues_per_port):
            if not self.bank.empty(port, i):
                return self.bank.dequeue(port, i)
        raise RuntimeError("tried to schedule a port with no packets")
    
    def non_empty_port_mask(self):
        return self.bank.non_empty_port_mask()

bank = PacketQueueBank(32, 32)
cla = PyTorClassifier()
qm = PyDropQueueManager()
sch = PyScheduler()

rtr = PyCompositeRouter(cla,qm,sch,1,0)

b = emu_packet()
b.src = 1
b.dst = 2
b.flow = 3

rtr.push(b)
c = pkt()
rtr.pull(0,c)