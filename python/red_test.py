#!/usr/bin/python

from fastemu import *

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
red_params = red_args()
red_params.q_capacity = 400;
red_params.ecn = False;
red_params.min_th = 20; # 200 microseconds
red_params.max_th = 200; # 2 milliseconds
red_params.max_p = 0.05;
red_params.wq_shift = 8;

rtr = REDRouter(0, red_params, dropper)

#make endpoint group
ENDPOINT_MAX_QUEUE_SIZE = 1 << 10 # should be power of two
epg = SimpleEndpointGroup(EMU_NUM_ENDPOINTS, emu_output, 0, ENDPOINT_MAX_QUEUE_SIZE)

# network driver
driver = SingleRackNetworkDriver(get_new_pkts_ring(state), epg, rtr,
                                 state.stat, PACKET_MEMPOOL_SIZE)

emu_add_backlog(state,0,1,0,300)

emu_add_backlog(state,5,6,0,400)

for i in xrange(1000):
    driver.step()
    emu_output.flush()
    
    admitted = get_admitted(state)
    if admitted is None:
        raise RuntimeError("expected to have admitted traffic after emu_output.flush()")
    
    print "finished traffic:"
    for i in xrange(admitted.size):
        edge = admitted_get_edge(admitted,i)
        if (edge.flags == EMU_FLAGS_DROP):
            print("\tDROP src %d to dst %d (id %d)" % 
                    (edge.src, edge.dst, edge.id))
        else:
            print("\tsrc %d to dst %d (id %d)" % 
                    (edge.src, edge.dst, edge.id))
