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

UPDATE: Each element in the timer heap is now a 3-tuple of form (time_to_send, dest_id, PDQ_layer). 
Now, when the PDQ layer updates its variables, the timer heap will reflect the changed rates. 

"""
class PyPDQQueueManager(PyQueueManager):
    # TODO: Calculate what to put here
    SHORT_DEADLINE = 0
    LONG_DEADLINE = 0
    THRESHOLD = 0

    def __init__(self, queue_bank, queue_capacity, dropper, endpoint_info_list, timer_heap_list):
        super(PyDropQueueManager, self).__init__()
        self.bank = queue_bank
        self.capacity = queue_capacity
        self.dropper = dropper
        self.endpoint_info_list = endpoint_info_list
        self.timer_heap_list = timer_heap_list

    def enqueue(self, pkt_p, port, queue):
        if self.bank.occupancy(port, queue) >= self.capacity:
            self.dropper.drop(pkt_p)
        else:
            # Add new flow if it doesn't exist, or update flow
            # with the most recently calculated deadline.
            self.update_flow(pkt_p)
            self.bank.enqueue(port, queue, pkt_p)

    """
    Adds info for a new flow to the PDQ layer flow set; 
    Inserts flow into the timer heap at the endpoint.
    """
    def update_flow(self, pkt_p):
        max_rate = 1
        expected_trans_time = None

        if self.bank.occupancy(port, queue) < THRESHOLD:
            deadline = SHORT_DEADLINE
        else:
            deadline = LONG_DEADLINE

        ep_id = pkt_p.dest

        time_to_send = 0

        updated_flow_state = FlowState(pkt_p.src, pkt_p.dest, max_rate, expected_trans_time, deadline)
        self.endpoint_info_list[pkt_p.src].PDQ_layer.flow_set[ep_id] = updated_flow_state

        timer_heap = self.timer_heap_list[pkt_p.src]
        heapq.heappush(timer_heap, (time_to_send, ep_id, self.endpoint_info_list[pkt_p.src].PDQ_layer.flow_set[ep_id]))


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

    def __init__(self, queue_bank, n_queues_per_port, endpoint_info_list, timer_heap_list):
        super(PyPDQEndpointScheduler, self).__init__()
        self.bank = queue_bank
        self.n_queues_per_port = n_queues_per_port
        self.endpoint_info_list = endpoint_info_list
        self.timer_heap_list = timer_heap_list

    def schedule(self, port):
        result = self.select_queue(self, port)
        if result is None:
            return None
        else:
            queue_id, time_to_send, flow_state = result
            packet = self.bank.dequeue(port, queue_id)
            self.check_for_finished_flow(port, queue_id)
            self.update_timer_heap(port, packet, time_to_send, flow_state)

            # add header to packet here
            self.add_header_to_packet(packet, port, queue_id)

            # Send packet through PDQ layer and update rate
            self.endpoint_info_list[port].PDQ_layer.process_outgoing_packet(packet, True)
            return packet


    """
    Calculates the deadline based on queue size; adds fields from PDQ's 
    flow set into packet header. 
    """
    def add_header_to_packet(self, packet, port, queue_id):

        flow_to_dest = self.endpoint_info_list[port].PDQ_layer.flow_set[packet.dest]

        #rate, max_rate, deadline, expected_trans_time = flow_to_dest

        if self.bank.occupancy(port, queue) < THRESHOLD:
            deadline = SHORT_DEADLINE
        else:
            deadline = LONG_DEADLINE

        # Add header here; temporarily setting them as subfields of packet..
        # need to figure out how to do this correctly. 
        packet.rate = flow_to_dest.R_s
        packet.max_rate = flow_to_dest.R_max
        packet.deadline = deadline
        packet.expected_trans_time = flow_to_dest.expected_trans_time

        self.PDQ_layer.flow_set[ep_id].deadline = deadline

    """
    Selects and returns a queue to transmit from, or None if it is nobody's turn
    to send. 
    Structure of PDQ_layer.flow_set dictionary is:
    key = endpoint id
    value = FlowState
    """
    def select_queue(self, port):
        # First remove flows that have missed their deadline. 
        while len(self.timer_heap)> 0 and self.timer_heap[0][2].deadline <= self.time:
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