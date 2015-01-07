#!/usr/bin/python

from fastemu import *

# allocate emulation state
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
dctcp_params = dctcp_args()
dctcp_params.q_capacity = 4096

#dctcp_params.K_threshold = 65  # from Alizadeh et al. SIGCOMM 2010. this might not be correct in general
dctcp_params.K_threshold = 20 # for 1 Gbit/s

rtr = DCTCPRouter(0, dctcp_params, dropper)

#make endpoint group
ENDPOINT_MAX_QUEUE_SIZE = 1 << 10 # should be power of two
epg = SimpleEndpointGroup(EMU_NUM_ENDPOINTS, emu_output, 0, ENDPOINT_MAX_QUEUE_SIZE)

# network driver
driver = SingleRackNetworkDriver(get_new_pkts_ring(state), epg, rtr,
                                 state.stat, PACKET_MEMPOOL_SIZE)

emu_add_backlog(state,0,1,0,100,0)
emu_add_backlog(state,2,1,0,200,0)
emu_add_backlog(state,3,1,0,100,0)

for i in xrange(420):
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
