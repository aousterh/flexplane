#!/usr/bin/python

from fastemu import *

"""
Scheduler for each PDQ router
There is only one output queue per port. 
Scheduler stores a set of flow state variables for each port.
Updates packet header with flow state vars as packet is dequeued / exits router.
"""
class PyPDQRouterScheduler(PyScheduler):
    def __init__(self, queue_bank, num_ports):
        super(PyPDQRouterScheduler, self).init()
        self.bank = queue_bank
        self.PDQ_layers = []
        for i in range(num_ports):
            self.PDQ_layers.append(PDQ_Layer())

    def schedule(self, port): # Assuming port arg is an index
        if not self.bank.empty(port, 0): # only 1 queue per port
            pkt =  self.bank.dequeue(port, 0) # Assuming dequeue func returns packet
            PDQ_layer = PDQ_layers[port]
            PDQ_layer.process_outgoing_packet(pkt, False)
            return pkt

"""
Scheduler for the PDQ endpoints. Each "port" given to schedule func is a different endpoint.
Each endpoint/port has n queues, one per destination.
Schedule function determines which queue (if any) to send from
in a given timeslot. 

Scheduler stores PDQ layers, timer heaps, and timers for each endpoint.
"""
class PyPDQEndpointScheduler(PyScheduler):
    def __init__(self, queue_bank, n_queues_per_port, flows=None):
        super(PyPDQEndpointScheduler, self).init()
        self.bank = queue_bank
        self.n_queues_per_port = n_queues_per_port
        self.endpoint_info_list = []
        for i in range(n_queues_per_port):
            timer_heap = None #### Change this -- Create timer heap with flows at that ep
            endpoint_info = EndpointInfo(PDQ_Layer(), timer_heap, 0)
            self.endpoint_info_list.append(endpoint_info)

    def schedule(self, port):
        ep_info = self.endpoint_info_list[port_index] #How do I get the port index? 
        return self.transmit(ep_info)

    def update_flows(self, flows): ### TODO: Replace "flows" with existing queue_bank
        self.flows = flows
        self.timer_heap = self.create_timer_heap()

    def create_timer_heap(self):
        timer_heap = []
        for dest_id, flow in self.flows.iteritems():
            heapq.heappush(timer_heap, (flow.time_to_send, flow))
        self.timer_heap = timer_heap

    """
    Transmits to a particular destination based on whose "turn" it is.
    Min-heap structure stores time-to-send, and when it is a flow's 
    time to send, it sends, and resets its countdown. 

    If a flow has missed its deadline, the flow is removed from the heap
    and the next-in-line flow attempts to transmit. 
    """
    def transmit(self, ep_info):
        if len(ep_info.timer_heap) == 0:
            return None
        if ep_info.timer_heap[0][0] <= ep_info.time:
            if ep_info.timer_heap[0][1].deadline <= ep_info.time:
                heapq.heappop(ep_info.timer_heap)
                self.transmit(ep_info)
            else:
                # Reset the counter
                (time_to_send, flow) = heapq.heappop(ep_info.timer_heap)
                if flow.dest.id not in ep_info.PDQ_layer.flow_set:
                    flow.R_s = 0
                else:
                    flow.R_s = ep_info.PDQ_layer.flow_set[flow.dest.id][0]

                if flow.R_s == 0: #Create new probe
                    probe_header = [flow.R_s, flow.R_max, flow.deadline, flow.expected_trans_time]
                    pkt = PDQ_Packet(flow.src, flow.dest, False, probe_header, True, False, False)
                    increment = 3
                else:
                    pkt = flow.flow_queue.pop(0)
                    [R_h_old, R_max, D_h, T_h] = pkt.header
                    pkt.header = [flow.R_s, R_max, D_h, T_h]
                    increment = int(1/flow.R_s)

                self.PDQ_layer.process_outgoing_packet(pkt, True)
                heapq.heappush(ep_info.timer_heap, (time_to_send + increment, flow))
                return pkt
            return None

"""
Helper for the PDQ Endpoint Scheduler - stores info for each endpoint. 
Time is an integer that keeps track of the current timeslot. 
"""
class EndpointInfo():
    def __init__(PDQ_layer, timer_heap, time):
        self.PDQ_Layer = PDQ_Layer
        self.timer_heap = timer_heap
        self.time = time


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

"""
PDQ is a layer attached to every output queue in the router. 
On packet exit from the router, PDQ recalculates flow info and
adjusts the packet's header. 
"""
class PDQ_Layer():
    def __init__(self):
        self.flow_set = {} # Stores dict of src_id: [flow_info]

    """
    Upon packet exit, determines whether to update router's flow variables.
    This function is called whenever a packet exits the router or endpoint.
    'source' arg is True if processing at source; false otherwise.  
    """
    def process_outgoing_packet(self, packet, source):
        if source == True: 
            flow_index = packet.dest.id
        else:
            flow_index = packet.src.id
        if packet.ack:
            pass
        elif packet.last: # Remove from flow set
            del self.flow_set[flow_index]
        else:
            self.update_flow_states(packet, flow_index)
            self.update_packet_header(packet, source, flow_index)
    
    def update_flow_states(self, packet, flow_index):
        self.flow_set[flow_index] = packet.header

        deadline_list = []
        for flow_dest, flow_info in self.flow_set.iteritems():
            deadline = flow_info[2]
            deadline_list.append((deadline, flow_dest))

        sorted_by_deadline = sorted(deadline_list)
        remaining_bw = 1

        # Give each flow some rate according to deadline
        while (remaining_bw > 0 and len(sorted_by_deadline) > 0):
            flow_dest = sorted_by_deadline.pop(0)[1]
            max_rate_for_flow_dest = self.flow_set[flow_dest][1]
            source_flow_rate = min(remaining_bw, max_rate_for_flow_dest) # Rate the source can give
            switch_flow_rate = self.flow_set[flow_dest][0] #whatever the rate was before
            
            new_flow_rate = min(source_flow_rate, switch_flow_rate)
            flow = self.flow_set[flow_dest]
            flow[0] = new_flow_rate
            self.flow_set[flow_dest] = flow
            remaining_bw = remaining_bw - new_flow_rate

        # Assign the rest of the flows rate 0
        while len(sorted_by_deadline) > 0:
            flow_dest = sorted_by_deadline.pop(0)[1]
            self.flow_set[flow_dest][0] = 0

    def update_packet_header(self, packet, source, flow_index):
        packet_flow = self.flow_set[flow_index]
        [R_h, R_max, D_h, T_h] = packet.header
        if source == True:
            packet.header = [1, R_max, D_h, T_h]
        else:
            packet.header = [packet_flow[0], R_max, D_h, T_h]


