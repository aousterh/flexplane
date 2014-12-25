/*
 * drop_tail.cc
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#include "drop_tail.h"
#include "api.h"
#include "api_impl.h"

#define DROP_TAIL_QUEUE_CAPACITY 4096

DropTailQueueManager::DropTailQueueManager(PacketQueueBank *bank,
		uint32_t queue_capacity, Dropper &dropper)
	: m_bank(bank), m_q_capacity(queue_capacity), m_dropper(dropper)
{
	if (bank == NULL)
		throw std::runtime_error("bank should be non-NULL");
}

void DropTailQueueManager::enqueue(struct emu_packet *pkt,
		uint32_t port, uint32_t queue)
{
	if (m_bank->occupancy(port, queue) >= m_q_capacity) {
		/* no space to enqueue, drop this packet */
		adm_log_emu_router_dropped_packet(&g_state->stat);
		m_dropper.drop(pkt);
	} else {
		m_bank->enqueue(port, queue, pkt);
	}
}

DropTailRouter::DropTailRouter(uint16_t id, uint16_t q_capacity,
		Dropper &dropper)
	: m_bank(EMU_ROUTER_NUM_PORTS, 1, DROP_TAIL_QUEUE_CAPACITY),
	  m_cla(16, 0, EMU_ROUTER_NUM_PORTS, 0),
	  m_qm(&m_bank, q_capacity, dropper),
	  m_sch(&m_bank),
	  DropTailRouterBase(&m_cla, &m_qm, &m_sch, EMU_ROUTER_NUM_PORTS, id)
{}

DropTailRouter::~DropTailRouter() {}

SimpleEndpoint::SimpleEndpoint(uint16_t id, struct simple_ep_args *args,
		EmulationOutput &emu_output)
	: Endpoint(id), m_emu_output(emu_output)
{
    uint32_t qmax;

    /* use args if supplied, otherwise use defaults */
    if (args != NULL)
        qmax = args->q_capacity;
    else
        qmax = DROP_TAIL_QUEUE_CAPACITY;
    
    queue_create(&output_queue, qmax);
}

SimpleEndpoint::~SimpleEndpoint() {
	reset();
}

void SimpleEndpoint::reset() {
	struct emu_packet *packet;

	/* dequeue all queued packets */
	while (queue_dequeue(&output_queue, &packet) == 0)
		m_emu_output.free_packet(packet);
}

SimpleEndpointGroup::SimpleEndpointGroup(uint16_t num_endpoints,
		EmulationOutput &emu_output, uint16_t start_id,
		struct simple_ep_args *args)
	: EndpointGroup(num_endpoints, emu_output)  {
	CONSTRUCT_ENDPOINTS(SimpleEndpoint, start_id, args, num_endpoints,
		endpoints, emu_output);
}

SimpleEndpointGroup::~SimpleEndpointGroup() {
	DESTRUCT_ENDPOINTS(num_endpoints, endpoints);
}

void SimpleEndpointGroup::reset(uint16_t endpoint_id) {
	RESET_ENDPOINT(endpoints, endpoint_id);
}

void SimpleEndpointGroup::new_packets(struct emu_packet **pkts,
		uint32_t n_pkts) {
	ENDPOINTS_NEW_PACKETS(SimpleEndpoint, new_packet, pkts, n_pkts,
			endpoints);
}

void SimpleEndpointGroup::push_batch(struct emu_packet **pkts,
		uint32_t n_pkts) {
	ENDPOINTS_PUSH_BATCH(SimpleEndpoint, push, pkts, n_pkts, endpoints);
}

uint32_t SimpleEndpointGroup::pull_batch(struct emu_packet **pkts,
		uint32_t n_pkts) {
	ENDPOINTS_PULL_BATCH(SimpleEndpoint, pull, pkts, n_pkts, random_state,
			num_endpoints, endpoints);
}
