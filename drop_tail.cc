/*
 * drop_tail.cc
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#include "drop_tail.h"
#include "api.h"
#include "api_impl.h"

#define DROP_TAIL_PORT_CAPACITY 128
#define DROP_TAIL_MAX_CAPACITY 256

inline uint32_t DropTailClassifier::classify(struct emu_packet *pkt) {
	return pkt->dst;
}

DropTailQueueManager::DropTailQueueManager(PacketQueueBank *bank,
		uint32_t queue_capacity)
	: m_bank(bank), m_q_capacity(queue_capacity)
{
	if (bank == NULL)
		throw std::runtime_error("bank should be non-NULL");
}

inline void DropTailQueueManager::enqueue(struct emu_packet *pkt,
		uint32_t queue_index)
{
	if (m_bank->occupancy(queue_index) >= m_q_capacity) {
		/* no space to enqueue, drop this packet */
		adm_log_emu_router_dropped_packet(&g_state->stat);
		drop_packet(pkt);
	} else {
		m_bank->enqueue(queue_index, pkt);
	}
}

DropTailRouter::DropTailRouter(uint16_t id, struct fp_ring *q_ingress,
		struct drop_tail_args *args)
	: m_bank(EMU_ROUTER_NUM_PORTS, 1, DROP_TAIL_MAX_CAPACITY),
	  m_cla(),
	  m_qm(&m_bank, ((args == NULL) ? DROP_TAIL_PORT_CAPACITY : args->port_capacity)),
	  m_sch(&m_bank),
	  DropTailRouterBase(&m_cla, &m_qm, &m_sch, id, q_ingress)
{}

DropTailRouter::~DropTailRouter() {
	uint16_t i;
	struct emu_packet *packet;

	/* free all queued packets */
	for (i = 0; i < EMU_ROUTER_NUM_PORTS; i++) {
		while (!m_bank.empty(i))
			free_packet(m_bank.dequeue(i));
	}
}

DropTailEndpoint::DropTailEndpoint(uint16_t id, struct drop_tail_args *args) :
		Endpoint(id) {
	uint32_t port_capacity;

	/* use args if supplied, otherwise use defaults */
	if (args != NULL)
		port_capacity = args->port_capacity;
	else
		port_capacity = DROP_TAIL_PORT_CAPACITY;

	queue_create(&output_queue, port_capacity);
}

DropTailEndpoint::~DropTailEndpoint() {
	reset();
}

void DropTailEndpoint::reset() {
	struct emu_packet *packet;

	/* dequeue all queued packets */
	while (queue_dequeue(&output_queue, &packet) == 0)
		free_packet(packet);
}

void DropTailEndpoint::new_packet(struct emu_packet *packet) {
	/* try to enqueue the packet */
	if (queue_enqueue(&output_queue, packet) != 0) {
		/* no space to enqueue, drop this packet */
		adm_log_emu_endpoint_dropped_packet(&g_state->stat);
		drop_packet(packet);
	}
}

void DropTailEndpoint::push(struct emu_packet *packet) {
	assert(packet->dst == id);

	/* pass the packet up the stack */
	enqueue_packet_at_endpoint(packet);
}

void DropTailEndpoint::pull(struct emu_packet **packet) {
	/* dequeue one incoming packet if the queue is non-empty */
	*packet = NULL;
	queue_dequeue(&output_queue, packet);
}
