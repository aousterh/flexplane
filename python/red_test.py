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


b = create_packet(state, 1,2,3,0) # state,src,dst,flow,id

print "sending packet", b
rtr.push(b)

c = rtr.pull(0)
print "null packet", c

c = rtr.pull(2)
print "the packet we sent (src,dst,flow,id) = ", c.src, c.dst, c.flow, c.id

