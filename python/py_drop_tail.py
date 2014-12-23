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
    def __init__(self, queue_bank, queue_capacity, dropper):
        # initialize the base class
        super(PyDropQueueManager, self).__init__()
        self.bank = queue_bank
        self.capacity = queue_capacity
        self.dropper = dropper
        
    def enqueue(self, pkt_p, port, queue):
        if self.bank.occupancy(port, queue) >= self.capacity:
            self.dropper.drop(pkt_p)
        else:
            self.bank.enqueue(port, queue, pkt_p)

class PyPrioScheduler(PyScheduler):
    def __init__(self, queue_bank, n_queues_per_port):
        super(PyPrioScheduler, self).__init__()
        self.bank = queue_bank
        self.n_queues_per_port = n_queues_per_port
        
    def schedule (self, port):
        for i in range(self.n_queues_per_port):
            if not self.bank.empty(port, i):
                return self.bank.dequeue(port, i)
        return None # no packets, shouldn't happen!
    
    def non_empty_port_mask(self):
        return self.bank.non_empty_port_mask()

# allocate router state
state = emu_state()
ADMITTED_MEMPOOL_SIZE = 1 << 10
ADMITTED_RING_SIZE = 1 << 10
PACKET_MEMPOOL_SIZE = 1 << 14
PACKET_RING_SIZE = 1 << 10

emu_alloc_init(state, ADMITTED_MEMPOOL_SIZE, ADMITTED_RING_SIZE,
               PACKET_MEMPOOL_SIZE, PACKET_RING_SIZE)

# make EmulationOutput and Dropper
emu_output = EmulationOutput(state.q_admitted_out, 
                             state.admitted_traffic_mempool, 
                             state.packet_mempool,
                             state.stat)
dropper = Dropper(emu_output)

# make router
NUM_ENDPOINTS = 32
NUM_QUEUES_PER_PORT = 4
MAX_QUEUE_CAPACITY_POW_2 = 256
DROP_TAIL_QUEUE_CAPCITY = 5 

bank = PacketQueueBank(NUM_ENDPOINTS, NUM_QUEUES_PER_PORT, MAX_QUEUE_CAPACITY_POW_2)
cla = PyTorClassifier()
qm = PyDropQueueManager(bank, DROP_TAIL_QUEUE_CAPCITY, dropper)
sch = PyPrioScheduler(bank, NUM_QUEUES_PER_PORT)

rtr = PyCompositeRouter(cla,qm,sch,NUM_ENDPOINTS,0)

#make endpoint group
epg = DropTailEndpointGroup(NUM_ENDPOINTS, emu_output)
epg.init(0, None)

b = create_packet(state, 1,2,3)

print "sending packet", b
rtr.push(b)

c = rtr.pull(0)
print "null packet", c

c = rtr.pull(2)
print "the packet we sent (src,dst,flow) = ", c.src, c.dst, c.flow

