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

Initialized with / stores the same list of EndpointInfo objects as the Schedulers and Sink.

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
            self.add_max_rate_to_header(pkt_p)

            # Add new flow if it doesn't exist, or update flow
            # with the most recently calculated deadline.
            self.update_flow(pkt_p)

            self.bank.enqueue(port, queue, pkt_p)

    def add_deadline_to_header(self, pkt_p, port, queue):
        if self.bank.occupancy(port, queue) < THRESHOLD:
            deadline = SHORT_DEADLINE
        else:
            deadline = LONG_DEADLINE
        ## TODO: Add deadline to packet header here. 

    """
    If max rate is not already a header field, add it and set it to 1. 
    """
    def add_max_rate_to_header(pkt_p):
        pass

    """
    Adds info for a new flow to the PDQ layer flow set, as well 
    as the timer heap. 
    """
    def update_flow(self, pkt_p):
        rate = 0
        max_rate = pkt_p.max_rate
        deadline = pkt_p.deadline
        expected_trans_time = None

        ep_id = pkt_p.dest
        flow_info = [rate, max_rate, deadline, expected_trans_time]
        self.PDQ_Layer.flow_set[ep_id] = flow_info


"""
Scheduler for the PDQ endpoints. Each "port" given to schedule() is a different endpoint.
Each endpoint has n queues, one per destination.
Schedule function determines which queue (if any) to send from
in a given timeslot. 

Then it updates the packet header and the PDQ data structures appropriately.

Scheduler stores a list of EndpointInfo objects, each of which contains
relevant data structures for an endpoint:
- PDQ_Layer
- Timer heap
- Timer
(QM and Sink store a pointer to the same list of EndpointInfo objects.)
"""
class PyPDQEndpointScheduler(PyScheduler):

    PROBE_FREQ = 3

    def __init__(self, queue_bank, n_queues_per_port, endpoint_info_list):
        super(PyPDQEndpointScheduler, self).__init__()
        self.bank = queue_bank
        self.n_queues_per_port = n_queues_per_port
        self.endpoint_info_list = endpoint_info_list

    def schedule(self, port):
        result = self.select_queue(self, port)
        if result is None:
            return None
        else:
            queue_id, time_to_send, flow_state = result
            packet = self.bank.dequeue(port, queue_id)
            self.check_for_finished_flow(port, queue_id)
            self.update_timer_heap(port, packet, time_to_send, flow_state)
            self.run_PDQ(port, packet)
            return packet

    """
    Selects and returns a queue to transmit from, or None if it is nobody's turn
    to send. 

    Each element in timer heap is a tuple of form:
    (flow.time_to_send, flow_state); flow.time_to_send is an integer and flow_state
    is a FlowState object.

    Structure of PDQ_layer.flow_set dictionary is:
    key = endpoint id
    value = flow_info, which is in the form [R_h, R_max, deadline, expected_trans_time]
    """
    def select_queue(self, port):
        # First remove flows that have missed their deadline. 
        while len(self.timer_heap)> 0 and self.timer_heap[0][1].deadline <= self.time:
            heapq.heappop(self.timer_heap)

        if len(self.timer_heap) == 0:
            return

        selected_flow_ep = self.timer_heap[0]

        if selected_flow_ep[0] <= self.time: # Time to send
            (time_to_send, flow_state) = heapq.heappop(self.timer_heap)
            queue_id = flow_state.dest_id
            return queue_id, time_to_send, flow_state 

    """
    Check if the queue to dest is now empty after dequeue-ing packet.
    If so, marks the packet as as TERM packet. 
    """
    def check_for_finished_flow(self, port, queue_id, packet):
        if self.bank.occupancy(port, queue_id) == 0:
            pass
            # TODO: Mark packet header with TERM field

    """
    Inserts flow back into the heap; or deletes it if packet was last in flow. 
    """
    def update_timer_heap(self, port, queue_id, packet, time_to_send, flow_state):
        if not packet.term: 
            if flow_state.R_s == 0:
                increment = PROBE_FREQ
            else:
                increment = 1/flow_state.R_s
            heapq.heappush(self.timer_heap, (time_to_send + increment, flow_state))

    """
    Send packet through PDQ layer and update rate.
    """
    def run_PDQ(self, port, packet):
        self.PDQ_layer.process_outgoing_packet(next_pkt, True)

"""
Scheduler for each PDQ router
There is only one output queue per port. 
Scheduler stores a set of flow state variables for each port.
Updates packet header with flow state vars as packet is dequeued / exits router.

Router Scheduler has different set of PDQ layers than the Endpoint Scheduler and QM.
"""
class PyPDQRouterScheduler(PyScheduler):
    def __init__(self, queue_bank, n_queues_per_port, PDQ_layers):
        super(PyPDQRouterScheduler, self).__init__()
        self.bank = queue_bank
        self.n_queues_per_port = 1
        self.PDQ_layers = PDQ_layers

    def schedule(self, port):
        if not self.bank.empty(port, 0): # only 1 queue per port
            pkt =  self.bank.dequeue(port, 0) # Assuming dequeue func returns packet
            PDQ_layer = self.PDQ_layers[port]
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