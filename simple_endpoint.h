/*
 * simple_endpoint.h
 *
 *  Created on: December 24, 2014
 *      Author: aousterh
 */

#ifndef SIMPLE_ENDPOINT_H_
#define SIMPLE_ENDPOINT_H_

#include "api.h"
#include "config.h"
#include "endpoint.h"
#include "endpoint_group.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/random.h"
#include "output.h"
#include "composite.h"
#include "classifiers/SingleQueueClassifier.h"
#include "queue_managers/drop_tail.h"

#define SIMPLE_ENDPOINT_QUEUE_CAPACITY 8192

struct simple_ep_args {
    uint16_t q_capacity;
};

/**
 * A SimpleSink just admits all packets when they reach the endpoint
 */
class SimpleSink : public Sink {
public:
	inline SimpleSink(uint16_t rack_id) : m_rack_id(rack_id) {}
	inline void assign_to_core(EmulationOutput *out) { m_output = out; }
	inline void handle(struct emu_packet *pkt) {
		assert((pkt->dst >> EMU_RACK_SHIFT) == m_rack_id);
		m_output->admit(pkt);
	}
private:
	EmulationOutput *m_output;
	uint16_t m_rack_id;
};

typedef CompositeEndpointGroup<SingleQueueClassifier, DropTailQueueManager, SingleQueueScheduler, SimpleSink>
	SimpleEndpointGroupBase;

/**
 * A drop tail endpoint group.
 */
class SimpleEndpointGroup : public SimpleEndpointGroupBase {
public:
	SimpleEndpointGroup(uint16_t num_endpoints, uint16_t start_id,
			uint16_t q_capacity);
	virtual void assign_to_core(EmulationOutput *out,
			struct emu_admission_core_statistics *stat);

	virtual void reset(uint16_t endpoint_id);
private:
    PacketQueueBank m_bank;
    EmulationOutput *m_emu_output;
    SingleQueueClassifier m_cla;
    DropTailQueueManager m_qm;
    SingleQueueScheduler m_sch;
    SimpleSink m_sink;
};

#endif /* SIMPLE_ENDPOINT_H_ */
