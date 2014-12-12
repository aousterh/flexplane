/*
 * composite.h
 *
 *  Created on: Dec 12, 2014
 *      Author: yonch
 */

#ifndef COMPOSITE_H_
#define COMPOSITE_H_

#include "packet.h"

/**
 * Classifier classes decide for a given packet, which queue they should
 *   go into.
 */
class Classifier {
public:
	/**
	 * @param pkt: packet to classify
	 * @return index of queue into which packet should go
	 */
	uint32_t classify(struct emu_packet *pkt);
};

/**
 * QueueManagers decide whether to accept or drop a packet, and how to maintain
 *   queues given such an incoming packet (does the queue drop packets from the
 *   tail or head? which queues drop packets, etc..)
 */
class QueueManager {
public:
	/**
	 * @param pkt: packet to enqueue
	 * @param queue_index: the index of the queue to use
	 */
	void enqueue(struct emu_packet *pkt, uint32_t queue_index);
};

/**
 * Schedulers choose, at each time-slot, what packet should leave the port
 */
class Scheduler {
public:
	/**
	 * @return the packet to transmit, or NULL if no packets should be transmitted
	 */
	struct emu_packet *schedule();
};

/**
 * A CompositeRouter is made of a Classifier, a QueueManager, N queues, and
 *   M Schedulers (one scheduler for each outgoing port).
 */
template < class CLA, class QM, class SCH >
class CompositeRouter : public Router {
public:
	CompositeRouter(CLA *cla, QM *qm, SCH *sch, uint32_t n_queues,
			uint32_t n_ports);
	virtual ~CompositeRouter();

	virtual void push(struct emu_packet *packet);
	virtual void pull(uint16_t output, struct emu_packet **packet);
};

#endif /* COMPOSITE_H_ */
