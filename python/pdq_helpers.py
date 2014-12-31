"""
Contains helper classes and functions for PDQ.
"""

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

    TODO: Update so that it does not need to handle deleting of flows
    (this is now handled at the endpoint)
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

"""
Helper for the PDQ Endpoint Scheduler - stores info for each endpoint. 
Time is an integer that keeps track of the current timeslot. 
"""
class EndpointInfo():
    def __init__(self, PDQ_layer, timer_heap, time):
        self.PDQ_Layer = PDQ_Layer
        self.timer_heap = timer_heap
        self.time = time



