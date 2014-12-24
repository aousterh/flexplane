/*
 * drop_tail.h
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#ifndef DROP_TAIL_H_
#define DROP_TAIL_H_

#include "api.h"
#include "config.h"
#include "packet_queue.h"
#include "router.h"
#include "endpoint.h"
#include "endpoint_group.h"
#include "composite.h"
#include "queue_bank.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/random.h"
#include "classifiers/TorClassifier.h"
#include "schedulers/SingleQueueScheduler.h"
#include "output.h"

#include <stdexcept>

struct packet_queue;

struct drop_tail_args {
    uint16_t q_capacity;
};


class DropTailQueueManager : public QueueManager {
public:
	DropTailQueueManager(PacketQueueBank *bank, uint32_t queue_capacity,
			Dropper &dropper);
	void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue);

private:
	/** the QueueBank where packets are stored */
	PacketQueueBank *m_bank;

	/** the maximum capacity of each single queue */
	uint32_t m_q_capacity;

	/** the means to drop packets */
	Dropper m_dropper;
};

typedef CompositeRouter<TorClassifier, DropTailQueueManager, SingleQueueScheduler>
	DropTailRouterBase;

/**
 * A drop tail router.
 * @output_queue: a queue of packets for each output port
 */
class DropTailRouter : public DropTailRouterBase {
public:
    DropTailRouter(uint16_t id, uint16_t q_capacity, Dropper &dropper);
    virtual ~DropTailRouter();

private:
    PacketQueueBank m_bank;
    TorClassifier m_cla;
    DropTailQueueManager m_qm;
    SingleQueueScheduler m_sch;
};

/**
 * A drop tail endpoint.
 * @output_queue: a queue of packets to be sent out
 */
class DropTailEndpoint : public Endpoint {
public:
	DropTailEndpoint(uint16_t id, struct drop_tail_args *args,
			EmulationOutput &emu_output);
	~DropTailEndpoint();
	virtual void reset();
	void inline new_packet(struct emu_packet *packet) {
		/* try to enqueue the packet */
		if (queue_enqueue(&output_queue, packet) != 0) {
			/* no space to enqueue, drop this packet */
			adm_log_emu_endpoint_dropped_packet(&g_state->stat);
			m_emu_output.drop(packet);
		}
	}
	void inline push(struct emu_packet *packet) {
		assert(packet->dst == id);

		/* pass the packet up the stack */
		m_emu_output.admit(packet);
	}
	void inline pull(struct emu_packet **packet) {
		/* dequeue one incoming packet if the queue is non-empty */
		*packet = NULL;
		queue_dequeue(&output_queue, packet);
	}
private:
	struct packet_queue	output_queue;
	EmulationOutput &m_emu_output;
};

/**
 * A drop tail endpoint group.
 */
class DropTailEndpointGroup : public EndpointGroup {
public:
	DropTailEndpointGroup(uint16_t num_endpoints, EmulationOutput &emu_output,
			uint16_t start_id, struct drop_tail_args *args);
	~DropTailEndpointGroup();
	virtual void reset(uint16_t endpoint_id);
	virtual void new_packets(struct emu_packet **pkts, uint32_t n_pkts);
	virtual void push_batch(struct emu_packet **pkts, uint32_t n_pkts);
	virtual uint32_t pull_batch(struct emu_packet **pkts, uint32_t n_pkts);
private:
	DropTailEndpoint	*endpoints[MAX_ENDPOINTS_PER_GROUP];
};

#endif /* DROP_TAIL_H_ */
