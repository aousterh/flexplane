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
#include "classifiers/TorClassifier.h"

#include <stdexcept>

struct packet_queue;

struct drop_tail_args {
	uint16_t port_capacity;
};


class DropTailQueueManager : public QueueManager {
public:
	DropTailQueueManager(PacketQueueBank *bank, uint32_t queue_capacity);
	inline void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue);

private:
	/** the QueueBank where packets are stored */
	PacketQueueBank *m_bank;

	/** the maximum capacity of each single queue */
	uint32_t m_q_capacity;
};


class DropTailScheduler : public Scheduler {
public:
	DropTailScheduler(PacketQueueBank *bank) : m_bank(bank) {}

	inline struct emu_packet *schedule(uint32_t output_port) {
		if (unlikely(m_bank->empty(output_port, 0)))
#ifdef EMU_NO_BATCH_CALLS
			return NULL;
#else
			throw std::runtime_error("called schedule on an empty port");
#endif
		else
			return m_bank->dequeue(output_port, 0);
	}

	inline uint64_t *non_empty_port_mask() {
		return m_bank->non_empty_port_mask();
	}

private:
	/** the QueueBank where packets are stored */
	PacketQueueBank *m_bank;
};

typedef CompositeRouter<TorClassifier, DropTailQueueManager, DropTailScheduler>
	DropTailRouterBase;

/**
 * A drop tail router.
 * @output_queue: a queue of packets for each output port
 */
class DropTailRouter : public DropTailRouterBase {
public:
	DropTailRouter(uint16_t id, struct drop_tail_args *args);
	virtual ~DropTailRouter();

private:
	PacketQueueBank m_bank;
	TorClassifier m_cla;
	DropTailQueueManager m_qm;
	DropTailScheduler m_sch;
};

/**
 * A drop tail endpoint.
 * @output_queue: a queue of packets to be sent out
 */
class DropTailEndpoint : public Endpoint {
public:
	DropTailEndpoint(uint16_t id, struct drop_tail_args *args);
	~DropTailEndpoint();
	virtual void reset();
	// TODO: make these bulk functions
	virtual void new_packet(struct emu_packet *packet);
	virtual void push(struct emu_packet *packet);
	virtual void pull(struct emu_packet **packet);
private:
	struct packet_queue	output_queue;
};

/**
 * A drop tail endpoint group.
 */
class DropTailEndpointGroup : public EndpointGroup {
public:
	DropTailEndpointGroup(uint16_t num_endpoints,
			struct fp_ring *q_new_packets, struct fp_ring *q_from_network,
			struct fp_ring *q_to_router) : EndpointGroup(num_endpoints,
					q_new_packets, q_from_network, q_to_router) {}
	~DropTailEndpointGroup() {};
	virtual Endpoint *make_endpoint(uint16_t id, struct drop_tail_args *args) {
		return new DropTailEndpoint(id, args);
	}
	/* TODO: replace new_packets/push/pull functions with more efficient batch
	 * versions */
};

#endif /* DROP_TAIL_H_ */
