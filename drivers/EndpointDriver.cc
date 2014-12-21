/*
 * EndpointDriver.cc
 *
 *  Created on: Dec 21, 2014
 *      Author: yonch
 */

#include "EndpointDriver.h"

#include <assert.h>
#include <stdint.h>
#include "../config.h"
#include "../endpoint_group.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"
#include "../api.h"
#include "../api_impl.h"

#define ENDPOINT_MAX_BURST	(EMU_NUM_ENDPOINTS * 2)

EndpointDriver::EndpointDriver(struct fp_ring* q_new_packets,
		struct fp_ring* q_to_router, struct fp_ring* q_from_router,
		EndpointGroup* epg, struct emu_admission_statistics *stat)
	: m_q_new_packets(q_new_packets),
	  m_q_to_router(q_to_router),
	  m_q_from_router(q_from_router),
	  m_epg(epg),
	  m_stat(stat)
{}

void EndpointDriver::step() {
	push();
	pull();
	process_new();
}

/**
 * Emulate push at a single endpoint group with index @index
 */

inline void EndpointDriver::push() {
	uint32_t n_pkts;
	struct emu_packet *pkts[ENDPOINT_MAX_BURST];

	/* dequeue packets from network, pass to endpoint group */
	n_pkts = fp_ring_dequeue_burst(m_q_from_router,
			(void **) &pkts[0], ENDPOINT_MAX_BURST);
	m_epg->push_batch(&pkts[0], n_pkts);
}

/**
 * Emulate pull at a single endpoint group with index @index
 */
inline void EndpointDriver::pull() {
	uint32_t n_pkts, i;
	struct emu_packet *pkts[MAX_ENDPOINTS_PER_GROUP];

	/* pull a batch of packets from the epg, enqueue to router */
	n_pkts = m_epg->pull_batch(&pkts[0], MAX_ENDPOINTS_PER_GROUP);
	assert(n_pkts <= MAX_ENDPOINTS_PER_GROUP);
	if (fp_ring_enqueue_bulk(m_q_to_router,
			(void **) &pkts[0], n_pkts) == -ENOBUFS) {
		/* enqueue failed, drop packets and log failure */
		for (i = 0; i < n_pkts; i++)
			drop_packet(pkts[i]);
		adm_log_emu_send_packets_failed(m_stat, n_pkts);
	} else {
		adm_log_emu_endpoint_sent_packets(m_stat, n_pkts);
	}
}

/**
 * Emulate new packets at a single endpoint group with index @index
 */
inline void EndpointDriver::process_new()
{
	uint32_t n_pkts;
	struct emu_packet *pkts[ENDPOINT_MAX_BURST];

	/* dequeue new packets, pass to endpoint group */
	n_pkts = fp_ring_dequeue_burst(m_q_new_packets,
			(void **) &pkts, ENDPOINT_MAX_BURST);
	m_epg->new_packets(&pkts[0], n_pkts);
}


