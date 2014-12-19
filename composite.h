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
#include <stdexcept>
#include "../graph-algo/platform.h"

#define THROW 	throw std::runtime_error("not implemented")
/**
 * Classifier classes decide for a given packet, which queue they should
 *   go into.
 */
class Classifier {
public:
	/**
	 * @param pkt: packet to classify
	 * @param port: [out] port where packet should leave
	 * @param queue: [out] index of per-port queue to enqueue packet
	 */
	void classify(struct emu_packet *pkt, uint32_t *port, uint32_t *queue) {THROW;}
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
	 * @param port: port to enqueue packet
	 * @param queue: index of per-port queue where packet should be enqueued
	 */
	void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue) {THROW;}
};

/**
 * Schedulers choose, at each time-slot, what packet should leave the port
 */
class Scheduler {
public:
	/**
	 * @return the packet to transmit, or NULL if no packets should be transmitted
	 */
	struct emu_packet *schedule(uint32_t output_port) {THROW;}

	/**
	 * @return a pointer to a bit mask with 1 for ports with packets, 0 o/w.
	 */
	uint64_t *non_empty_port_mask() {THROW;}
};

#undef THROW

/**
 * A CompositeRouter is made of a Classifier, a QueueManager and a Scheduler.
 */
template < class CLA, class QM, class SCH >
class CompositeRouter : public Router {
public:
	CompositeRouter(CLA *cla, QM *qm, SCH *sch, uint32_t n_ports, uint16_t id);
	virtual ~CompositeRouter();

	virtual void push(struct emu_packet *packet);
	virtual void pull(uint16_t port, struct emu_packet **packet);

	virtual void push_batch(struct emu_packet **pkts, uint32_t n_pkts);
	virtual uint32_t pull_batch(struct emu_packet **pkts, uint32_t n_pkts);

private:
	CLA *m_cla;
	QM *m_qm;
	SCH *m_sch;
	uint32_t m_n_ports;
};

/** implementation */
template < class CLA, class QM, class SCH >
CompositeRouter<CLA,QM,SCH>::CompositeRouter(
		CLA *cla, QM *qm, SCH *sch, uint32_t n_ports, uint16_t id)
	: Router(id),
	  m_cla(cla), m_qm(qm), m_sch(sch),
	  m_n_ports(n_ports)
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
	uint32_t port, queue;
	m_cla->classify(packet, &port, &queue);
	m_qm->enqueue(packet, port, queue);
}

template < class CLA, class QM, class SCH >
void CompositeRouter<CLA,QM,SCH>::pull(uint16_t port,
		struct emu_packet **packet)
{
	*packet = m_sch->schedule(port);
}

template < class CLA, class QM, class SCH >
void CompositeRouter<CLA,QM,SCH>::push_batch(struct emu_packet **pkts,
		uint32_t n_pkts)
{
	for (uint32_t i = 0; i < n_pkts; i++) {
		uint32_t port, queue;
		m_cla->classify(pkts[i], &port, &queue);
		m_qm->enqueue(pkts[i], port, queue);
	}
}

template < class CLA, class QM, class SCH >
uint32_t CompositeRouter<CLA,QM,SCH>::pull_batch(struct emu_packet **pkts,
		uint32_t n_pkts)
{
	uint64_t *non_empty_port_mask = m_sch->non_empty_port_mask();
	uint32_t res = 0;

	if (unlikely(n_pkts < m_n_ports))
		throw std::runtime_error("pull_batch should be passed space for at least n_ports packets");

	for (uint32_t i = 0; i < ((m_n_ports + 63) >> 6); i++) {
		uint64_t mask = non_empty_port_mask[i];
		uint64_t port;
		while (mask) {
			/* get the index of the lsb that is set */
			asm("bsfq %1,%0" : "=r"(port) : "r"(mask));
			/* turn off the set bit in the mask */
			mask &= (mask - 1);
			port += 64 * i;

			pkts[res++] = m_sch->schedule(port);
		}
	}

	return res;
}

#endif /* COMPOSITE_H_ */
