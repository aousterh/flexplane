/*
 * composite.h
 *
 *  Created on: Dec 12, 2014
 *      Author: yonch
 */

#ifndef COMPOSITE_H_
#define COMPOSITE_H_

#include "packet.h"
#include <stdio.h>

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
	struct emu_packet *schedule(uint32_t output_port);

	/**
	 * schedules a batch of packets
	 * @param pkts: [out] an array to store scheduled packets
	 * @param n_pkts: the maximum number of packets to schedule
	 *
	 */
	uint32_t schedule_batch(struct emu_packet **pkts, uint32_t n_pkts);
};

/**
 * A CompositeRouter is made of a Classifier, a QueueManager and a Scheduler.
 */
template < class CLA, class QM, class SCH >
class CompositeRouter : public Router {
public:
	CompositeRouter(CLA *cla, QM *qm, SCH *sch, uint16_t id,
			struct fp_ring *q_ingress);
	virtual ~CompositeRouter();

	virtual void push(struct emu_packet *packet);
	virtual void pull(uint16_t output, struct emu_packet **packet);

	virtual void push_batch(struct emu_packet **pkts, uint32_t n_pkts);
	virtual uint32_t pull_batch(struct emu_packet **pkts, uint32_t n_pkts);

private:
	CLA *m_cla;
	QM *m_qm;
	SCH *m_sch;
};

/** implementation */
template < class CLA, class QM, class SCH >
CompositeRouter<CLA,QM,SCH>::CompositeRouter(
		CLA *cla, QM *qm, SCH *sch, uint16_t id, struct fp_ring *q_ingress)
	: Router(id, q_ingress),
	  m_cla(cla), m_qm(qm), m_sch(sch)
{
	/* static check: make sure template parameters are of the correct classes */
	(void)static_cast<Classifier*>((CLA*)0);
	(void)static_cast<QueueManager*>((QM*)0);
	(void)static_cast<Scheduler*>((SCH*)0);
}

template < class CLA, class QM, class SCH >
CompositeRouter<CLA,QM,SCH>::~CompositeRouter() {}

template < class CLA, class QM, class SCH >
void CompositeRouter<CLA,QM,SCH>::push(struct emu_packet *packet)
{
	int32_t queue_index = m_cla->classify(packet);
	m_qm->enqueue(packet, queue_index);
}

template < class CLA, class QM, class SCH >
void CompositeRouter<CLA,QM,SCH>::pull(uint16_t output,
		struct emu_packet **packet)
{
	*packet = m_sch->schedule(output);
}

template < class CLA, class QM, class SCH >
void CompositeRouter<CLA,QM,SCH>::push_batch(struct emu_packet **pkts,
		uint32_t n_pkts)
{
	for (uint32_t i = 0; i < n_pkts; i++) {
		int32_t queue_index = m_cla->classify(pkts[i]);
		m_qm->enqueue(pkts[i], queue_index);
	}
}

template < class CLA, class QM, class SCH >
uint32_t CompositeRouter<CLA,QM,SCH>::pull_batch(struct emu_packet **pkts,
		uint32_t n_pkts)
{
	return m_sch->schedule_batch(pkts, n_pkts);
}

#endif /* COMPOSITE_H_ */
