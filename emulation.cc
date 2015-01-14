/*
 * emulation.cc
 *
 *  Created on: June 24, 2014
 *      Author: aousterh
 */

#include "emulation.h"
#include "api.h"
#include "api_impl.h"
#include "admitted.h"
#include "endpoint_group.h"
#include "router.h"
#include "drivers/EndpointDriver.h"
#include "drivers/RouterDriver.h"
#include "output.h"
#include "../protocol/topology.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#include <assert.h>
#include <stdexcept>

emu_state *g_state; /* global emulation state */

static inline void free_packet_ring(struct emu_state *state,
		struct fp_ring *packet_ring);

void emu_init_state(struct emu_state *state,
		struct fp_mempool *admitted_traffic_mempool,
		struct fp_ring *q_admitted_out, struct fp_mempool *packet_mempool,
	    struct fp_ring **packet_queues, RouterType r_type, void *r_args,
		EndpointType e_type, void *e_args) {
	uint32_t i, pq;
	uint32_t size;
	EndpointGroup *epg;
	Router *rtr;

	g_state = state;

	pq = 0;
	state->admitted_traffic_mempool = admitted_traffic_mempool;
	state->q_admitted_out = q_admitted_out;
	state->packet_mempool = packet_mempool;

	state->out = new EmulationOutput(state->q_admitted_out,
			state->admitted_traffic_mempool, state->packet_mempool,
			&state->stat);

	/* construct topology: 1 router with 1 rack of endpoints */

	/* initialize rings */
	for (i = 0; i < EMU_NUM_ROUTERS; i++) {
		state->q_router_ingress[i] = packet_queues[pq++];
	}
	state->comm_state.q_epg_new_pkts[0] = packet_queues[pq++];
	state->q_epg_ingress[0] = packet_queues[pq++];

	Dropper dropper(*state->out, &state->queue_bank_stats);

	/* initialize all the routers */
	for (i = 0; i < EMU_NUM_ROUTERS; i++) {
		/* clear queue bank stats */
		memset(&state->queue_bank_stats, 0, sizeof(struct queue_bank_stats));

		rtr = RouterFactory::NewRouter(r_type, r_args, i, dropper,
				&state->queue_bank_stats);
		assert(rtr != NULL);
		state->router_drivers[i] = new RouterDriver(rtr,
				state->q_router_ingress[0], state->q_epg_ingress[0],
				&state->stat);
	}

	/* initialize all the endpoints in one endpoint group */
	epg = EndpointGroupFactory::NewEndpointGroup(e_type, EMU_NUM_ENDPOINTS,
			*state->out, 0, e_args);

	assert(epg != NULL);
	state->endpoint_drivers[0] =
			new EndpointDriver(state->comm_state.q_epg_new_pkts[0],
					state->q_router_ingress[0], state->q_epg_ingress[0], epg,
					&state->stat);
}

void emu_cleanup(struct emu_state *state) {
	uint32_t i;
	struct emu_admitted_traffic *admitted;

	/* free all endpoints */
	for (i = 0; i < EMU_NUM_ENDPOINT_GROUPS; i++) {
		delete state->endpoint_drivers[i];

		/* free packet queues, return packets to mempool */
		free_packet_ring(state, state->comm_state.q_epg_new_pkts[i]);
		free_packet_ring(state, state->q_epg_ingress[i]);
	}

	/* free all routers */
	for (i = 0; i < EMU_NUM_ROUTERS; i++) {
		delete state->router_drivers[i];

		/* free ingress queue for this router, return packets to mempool */
		free_packet_ring(state, state->q_router_ingress[i]);
	}

	delete state->out;

	/* empty queue of admitted traffic, return structs to the mempool */
	while (fp_ring_dequeue(state->q_admitted_out, (void **) &admitted) == 0)
		fp_mempool_put(state->admitted_traffic_mempool, admitted);
	fp_free(state->q_admitted_out);

	fp_free(state->admitted_traffic_mempool);
	fp_free(state->packet_mempool);
}



void emu_emulate(struct emu_state *state) {
	uint32_t i;

	/* push/pull at endpoints and routers must be done in a specific order to
	 * ensure that packets pushed in one timeslot cannot be pulled until the
	 * next. */

	/* push new packets from the network to endpoints */
	for (i = 0; i < EMU_NUM_ENDPOINT_GROUPS; i++)
		state->endpoint_drivers[i]->step();

	/* emulate one timeslot at each router (push and pull) */
	for (i = 0; i < EMU_NUM_ROUTERS; i++)
		state->router_drivers[i]->step();

	state->out->flush();
}

void emu_reset_sender(struct emu_state *state, uint16_t src) {
	state->endpoint_drivers[0]->reset_endpoint(src);
}

/* frees all the packets in an fp_ring, and frees the ring itself */
static inline void free_packet_ring(struct emu_state *state,
		struct fp_ring *packet_ring)
{
	struct emu_packet *packet;

	while (fp_ring_dequeue(packet_ring, (void **) &packet) == 0) {
		free_packet(state, packet);
	}
	fp_free(packet_ring);
}

#ifdef NO_DPDK
void emu_alloc_init(struct emu_state* state, uint32_t admitted_mempool_size,
		uint32_t admitted_ring_size, uint32_t packet_mempool_size,
		uint32_t packet_ring_size)
{
	struct fp_mempool *admitted_traffic_mempool =
			fp_mempool_create(admitted_mempool_size,
					sizeof(struct emu_admitted_traffic));
	if (admitted_traffic_mempool == NULL)
		throw std::runtime_error("couldn't allocate admitted_traffic_mempool");

	struct fp_ring *q_admitted_out = fp_ring_create("q_admitted_out",
			admitted_ring_size, 0, 0);
	if (q_admitted_out == NULL)
		throw std::runtime_error("couldn't allocate q_admitted_out");

	struct fp_mempool *packet_mempool = fp_mempool_create(packet_mempool_size,
			EMU_ALIGN(sizeof(struct emu_packet)));
	if (packet_mempool == NULL)
		throw std::runtime_error("couldn't allocate packet_mempool");

	struct fp_ring *q_new_packets = fp_ring_create("q_new_packets",
			packet_ring_size, 0, 0);
	if (q_new_packets == NULL)
		throw std::runtime_error("couldn't allocate q_new_packets");

	struct fp_ring *packet_queues[EMU_NUM_PACKET_QS];
	packet_queues[1] = q_new_packets;

	emu_init_state(state, admitted_traffic_mempool, q_admitted_out,
			packet_mempool, packet_queues, R_DropTail, NULL,
			E_Simple, NULL);
}
#endif
