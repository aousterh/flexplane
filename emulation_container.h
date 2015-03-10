/*
 * emulation_container.h
 *
 *  Created on: March 5, 2015
 *      Author: aousterh
 */

#ifndef EMULATION_CONTAINER_H
#define EMULATION_CONTAINER_H

#include "admitted.h"
#include "emulation.h"
#include "endpoint.h"
#include "router.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"
#include <stdexcept>

/**
 * A class for containing an emulation and all communication in and out of it,
 * 	to facilitate testing outside the arbiter.
 */
class EmulationContainer {
public:
	EmulationContainer(uint32_t admitted_mempool_size,
			uint32_t admitted_ring_size, uint32_t packet_mempool_size,
			uint32_t packet_ring_size, enum RouterType r_type, void *r_args,
			enum EndpointType e_type, void *e_args);
	~EmulationContainer();

	inline void add_backlog(uint16_t src, uint16_t dst, uint16_t flow,
			uint32_t amount, uint16_t start_id, u8* areq_data);
	inline void step();
	inline void print_admitted();
private:
	Emulation			*m_emulation;
	struct fp_mempool	*m_admitted_mempool;
	struct fp_ring		*m_q_admitted_out;
	struct fp_mempool	*m_packet_mempool;
};

EmulationContainer::EmulationContainer(uint32_t admitted_mempool_size,
		uint32_t admitted_ring_size, uint32_t packet_mempool_size,
		uint32_t packet_ring_size, enum RouterType r_type, void *r_args,
		enum EndpointType e_type, void *e_args) {
	m_admitted_mempool = fp_mempool_create("admitted_mempool",
			admitted_mempool_size, sizeof(struct emu_admitted_traffic), 0, 0,
			0);
	if (m_admitted_mempool == NULL)
		throw std::runtime_error("couldn't allocate admitted_traffic_mempool");

	m_q_admitted_out = fp_ring_create("q_admitted_out", admitted_ring_size, 0,
			0);
	if (m_q_admitted_out == NULL)
		throw std::runtime_error("couldn't allocate q_admitted_out");

	m_packet_mempool = fp_mempool_create("packet_mempool", packet_mempool_size,
			EMU_ALIGN(sizeof(struct emu_packet)), 0, 0, 0);
	if (m_packet_mempool == NULL)
		throw std::runtime_error("couldn't allocate packet_mempool");

	m_emulation = new Emulation(m_admitted_mempool, m_q_admitted_out,
			m_packet_mempool, packet_ring_size, r_type, r_args, e_type,
			e_args);
}

EmulationContainer::~EmulationContainer() {
	m_emulation->cleanup();
	delete m_emulation;
}

inline void EmulationContainer::add_backlog(uint16_t src, uint16_t dst,
		uint16_t flow, uint32_t amount, uint16_t start_id, u8* areq_data) {
	m_emulation->add_backlog(src, dst, flow, amount, start_id, areq_data);
}

inline void EmulationContainer::step() {
	struct emu_admitted_traffic *admitted;

	/* dequeue any remaining admitted structs and return them to the mempool */
	while (fp_ring_dequeue(m_q_admitted_out, (void **) &admitted) == 0) {
		fp_mempool_put(m_admitted_mempool, admitted);
	}

	/* step emulation by one timeslot */
	m_emulation->step();
}

inline void EmulationContainer::print_admitted() {
	struct emu_admitted_traffic *admitted;

	/* print the admitted traffic */
	while (fp_ring_dequeue(m_q_admitted_out, (void **) &admitted) == 0) {
		admitted_print(admitted);
		fp_mempool_put(m_admitted_mempool, admitted);
	}
}

#endif /* EMULATION_CONTAINER_H */
