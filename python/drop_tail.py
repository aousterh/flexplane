#!/usr/bin/python

from fastemu import *

class PyTorRoutingTable(PyRoutingTable):
    def route(self, pkt_p):
        # need to convert from (struct emu_packet *) to (struct emu_packet)
        pkt = pkt_value(pkt_p)
        return pkt.dst

class PyFlowIDClassifier(PyClassifier):
    def classify(self, pkt_p, port):
        # need to convert from (struct emu_packet *) to (struct emu_packet)
        pkt = pkt_value(pkt_p)
        return pkt.flow 

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

class PySimpleSink(PySink):
    def __init__(self, emu_output):
        super(PySimpleSink, self).__init__()
        self.emu_output = emu_output
        
    def handle(self, pkt_p):
        self.emu_output.admit(pkt_p)

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
dropper = Dropper(emu_output, state.queue_bank_stats)

# make router
NUM_ENDPOINTS = 32
NUM_QUEUES_PER_PORT = 4
MAX_QUEUE_CAPACITY_POW_2 = 256
DROP_TAIL_QUEUE_CAPCITY = 5 

bank = PacketQueueBank(NUM_ENDPOINTS, NUM_QUEUES_PER_PORT, MAX_QUEUE_CAPACITY_POW_2, state.queue_bank_stats)
rt = PyTorRoutingTable()
cla = PyFlowIDClassifier()
qm = PyDropQueueManager(bank, DROP_TAIL_QUEUE_CAPCITY, dropper)
sch = PyPrioScheduler(bank, NUM_QUEUES_PER_PORT)

rtr = PyRouter(rt,cla,qm,sch,NUM_ENDPOINTS)

#make endpoint group
epg_bank = PacketQueueBank(NUM_ENDPOINTS, NUM_QUEUES_PER_PORT, MAX_QUEUE_CAPACITY_POW_2, state.queue_bank_stats)
epg_cla = PyFlowIDClassifier()
epg_qm = PyDropQueueManager(epg_bank, DROP_TAIL_QUEUE_CAPCITY, dropper)
epg_sch = PyPrioScheduler(epg_bank, NUM_QUEUES_PER_PORT)
epg_sink = PySimpleSink(emu_output)

epg = PyEndpointGroup(epg_cla, epg_qm, epg_sch, epg_sink, 0, NUM_ENDPOINTS)


# network driver
driver = SingleRackNetworkDriver(get_new_pkts_ring(state), epg, rtr, get_reset_ring(state),
                                 state.stat, PACKET_MEMPOOL_SIZE)

emu_add_backlog(state,0,1,0,30,0,None)

emu_add_backlog(state,5,6,0,40,0,None)

for i in xrange(1000):
    driver.step()
    emu_output.flush()
    
    print "finished traffic:"
    while (1):
        admitted = get_admitted(state)
        if admitted is None:
            raise RuntimeError("expected to have admitted traffic after emu_output.flush()")
        
        for i in xrange(admitted.size):
            edge = admitted_get_edge(admitted,i)
            if (edge.flags == EMU_FLAGS_DROP):
                print("\tDROP src %d to dst %d (id %d)" % 
                        (edge.src, edge.dst, edge.id))
            else:
                print("\tsrc %d to dst %d (id %d)" % 
                        (edge.src, edge.dst, edge.id))
        
        if admitted.size != EMU_NUM_ENDPOINTS + EMU_MAX_DROPS:
            break # break from the while loop, done with this timeslot
        
        # else, the admitted struct was full, so another will be coming from
        # the same timeslot
