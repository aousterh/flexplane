#!/usr/bin/python

from fastemu import *
from pdq_helpers import *

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

"""
PDQ Queue Manager
Sets/ resets flow deadline in packet header based on queue length.

Stores the same list of EndpointInfo objects as the Schedulers and Sink.

Adds new flows to flow set & timer heap. 
"""
class PyPDQQueueManager(PyQueueManager):
    # TODO: Calculate what to put here
    SHORT_DEADLINE = 0
    LONG_DEADLINE = 0
    THRESHOLD = 0

    def __init__(self, queue_bank, queue_capacity, dropper, endpoint_info_list):
        super(PyDropQueueManager, self).__init__()
        self.bank = queue_bank
        self.capacity = queue_capacity
        self.dropper = dropper
        self.endpoint_info_list = endpoint_info_list

    def enqueue(self, pkt_p, port, queue):
        if self.bank.occupancy(port, queue) >= self.capacity:
            self.dropper.drop(pkt_p)
        else:
            self.add_deadline_to_header(pkt_p, port, queue)

            # Add new flow if necessary
            if pkt_p.dest not in self.endpoint_info_list[pkt_p.src]:
                self.add_new_flow_(pkt_p)

            self.bank.enqueue(port, queue, pkt_p)

    def add_deadline_to_header(self, pkt_p, port, queue):
        if self.bank.occupancy(port, queue) < THRESHOLD:
            deadline = SHORT_DEADLINE
        else:
            deadline = LONG_DEADLINE
        ## TODO: Add deadline to packet header here. 

    """
    Adds info for a new flow to the PDQ layer flow set, as well 
    as the timer heap. 
    """
    def add_new_flow(self, pkt_p):
        pass

"""
Scheduler for the PDQ endpoints. Each "port" given to schedule() is a different endpoint.
Each endpoint has n queues, one per destination.
Schedule function determines which queue (if any) to send from
in a given timeslot. 

Scheduler stores a list of EndpointInfo objects, each of which contains
relevant data structures for an endpoint:
- PDQ_Layer
- Timer heap
- Timer
(QM and Sink store a pointer to the same list of EndpointInfo objects.)

Removes finished flows from timer heap. 
"""
class PyPDQEndpointScheduler(PyScheduler):
    def __init__(self, queue_bank, n_queues_per_port, endpoint_info_list):
        super(PyPDQEndpointScheduler, self).__init__()
        self.bank = queue_bank
        self.n_queues_per_port = n_queues_per_port
        self.endpoint_info_list = endpoint_info_list

    def schedule(self, port):
        queue_id = self.select_queue(self, port) #select internally updates PDQ_layer
        if queue_id == None:
            return None

        else:
            return self.bank.dequeue(port, queue_id)
            self.check_for_finished_flow(port, queue_id)

    """
    This was originally transmit() function.

    Returns a queue to transmit from, or None if it is nobody's turn
    to send. 
    """
    def select_queue(self, port):
        pass

    """
    If the just-dequeued packet was the last in that queue, 
    the flow has finished; removes it from timer heap and 
    PDQ layer flow set. 
    """
    def check_for_finished_flow(self, port, queue_id):
        pass

"""
Scheduler for each PDQ router
There is only one output queue per port. 
Scheduler stores a set of flow state variables for each port.
Updates packet header with flow state vars as packet is dequeued / exits router.
"""
class PyPDQRouterScheduler(PyScheduler):
    def __init__(self, queue_bank, n_queues_per_port, endpoint_info_list):
        super(PyPDQRouterScheduler, self).__init__()
        self.bank = queue_bank
        self.n_queues_per_port = 1
        self.endpoint_info_list = endpoint_info_list

    def schedule(self, port):
        if not self.bank.empty(port, 0): # only 1 queue per port
            pkt =  self.bank.dequeue(port, 0) # Assuming dequeue func returns packet
            PDQ_layer = PDQ_layers[port]
            PDQ_layer.process_outgoing_packet(pkt, False)
            return pkt    

"""
Handle function receives incoming packet - If packet is an ack, 
it updates endpoint variables accordingly. If packet is a data pkt,
it sends the corresponding ack. 
"""
class PyPDQSink(PySink):
    def __init__(self, emu_output):
        super(PySimpleSink, self).__init__()
        self.emu_output = emu_output

    def handle(self, pkt_p):
        if pkt_p.ack:
            self.received_ack_update(pkt_p)
        else:
            self.send_ack(pkt_p)
            self.emu_output.admit(pkt_p)

    def send_ack(self, pkt):
        pass

    def received_ack_update(self, ack):
        pass