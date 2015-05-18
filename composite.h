/*
 * composite.h
 *
 *  Created on: Dec 12, 2014
 *      Author: yonch
 */

#ifndef COMPOSITE_H_
#define COMPOSITE_H_

#include <stdio.h>
#include <stdexcept>
#include "packet.h"
#include "packet_impl.h"
#include "endpoint_group.h"
#include "router.h"
#include "../graph-algo/platform.h"
#include "../graph-algo/random.h"

#define THROW 	throw std::runtime_error("not implemented")

/**
 * Routing tables choose the outgoing port for a given packet
 */
class RoutingTable {
public:
	/**
	 * @param pkt: the packet to route
	 * @returns the port out of which packet should be transmitted
	 */
	uint32_t route(struct emu_packet *pkt) {THROW;}
};

/**
 * Classifiers decide for a given packet and its output port, which queue the
 *   packet should go into.
 */
class Classifier {
public:
	/**
	 * @param pkt: packet to classify
	 * @param port: port where packet will be output
	 * @returns the index of the per-port queue to enqueue packet
	 */
	uint32_t classify(struct emu_packet *pkt, uint32_t port) {THROW;}
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
	void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue,
			uint64_t cur_time) {THROW;}

	/**
	 * Prepare this queue manager to run on a specific core.
	 */
	void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat) {THROW;}

	/**
	 * Return a pointer to the port drop statistics.
	 */
	struct port_drop_stats *get_port_drop_stats() {THROW;}
};

/**
 * Schedulers choose, at each time-slot, what packet should leave the port
 */
class Scheduler {
public:
	/**
	 * @return the packet to transmit, or NULL if no packets should be transmitted
	 */
	struct emu_packet *schedule(uint32_t output_port, uint64_t cur_time) {
		THROW;}

	/**
	 * @return a pointer to a bit mask with 1 for ports with packets, 0 o/w.
	 */
	uint64_t *non_empty_port_mask() {THROW;}
};

/**
 * Sinks handle received packets at endpoints
 */
class Sink {
public:
	/**
	 * @param pkt: packet to handle
	 */
	void handle(struct emu_packet *pkt) {THROW;}
};

#undef THROW

/**
 * A CompositeRouter is made of a Routing Table, a Classifier, a QueueManager,
 * 		and a Scheduler.
 */
template < class RT, class CLA, class QM, class SCH >
class CompositeRouter : public Router {
public:
    CompositeRouter(RT *rt, CLA *cla, QM *qm, SCH *sch, uint32_t n_ports);
    virtual ~CompositeRouter();

    virtual void push(struct emu_packet *packet, uint64_t cur_time);
    virtual struct emu_packet *pull(uint16_t port, uint64_t cur_time);

    virtual void push_batch(struct emu_packet **pkts, uint32_t n_pkts,
    		uint64_t cur_time);
    virtual uint32_t pull_batch(struct emu_packet **pkts, uint32_t n_pkts,
    		uint64_t *port_masks, uint64_t cur_time);

	struct port_drop_stats *get_port_drop_stats() {
		return m_qm->get_port_drop_stats();
	}

private:
    RT *m_rt;
	CLA *m_cla;
	QM *m_qm;
	SCH *m_sch;
	uint32_t m_n_ports;
};

/**
 * A CompositeEndpointGroup is made of a Classifier, a QueueManager, a
 *   Scheduler, and a Sink.
 */
template < class CLA, class QM, class SCH, class SINK >
class CompositeEndpointGroup : public EndpointGroup {
public:
	CompositeEndpointGroup(CLA *cla, QM *qm, SCH *sch, SINK *sink,
			uint32_t first_endpoint_id, uint32_t n_endpoints);
	virtual ~CompositeEndpointGroup();

	virtual void new_packets(struct emu_packet **pkts, uint32_t n_pkts,
			uint64_t cur_time);
	virtual void push_batch(struct emu_packet **pkts, uint32_t n_pkts);
	virtual uint32_t pull_batch(struct emu_packet **pkts, uint32_t n_pkts,
			uint64_t cur_time);

private:
	CLA *m_cla;
	QM *m_qm;
	SCH *m_sch;
	SINK *m_sink;
	uint32_t m_first_endpoint_id;
	uint32_t m_n_endpoints;
};

/** shared functionality between CompositeEndpointGroup and CompositeRouter */
template < class SCH >
inline  __attribute__((always_inline))
uint32_t composite_pull_batch(SCH *sch, uint32_t n_elems,
		struct emu_packet **pkts, uint32_t n_pkts, uint64_t *port_masks,
		uint64_t cur_time)
{
	uint64_t *non_empty_port_mask = sch->non_empty_port_mask();
	uint32_t res = 0;

/*	if (unlikely(n_pkts < n_elems))
		throw std::runtime_error("pull_batch should be passed space for at least n_elems packets");*/

	for (uint32_t i = 0; i < ((n_elems + 63) >> 6); i++) {
		/* only pull from non-empty ports that are requested right now */
		uint64_t mask = non_empty_port_mask[i] & port_masks[i];
		uint64_t port;
		while (mask) {
			/* get the index of the lsb that is set */
			asm("bsfq %1,%0" : "=r"(port) : "r"(mask));
			/* turn off the set bit in the mask */
			mask &= (mask - 1);
			port += 64 * i;

			pkts[res++] = sch->schedule(port, cur_time);
		}
	}

	return res;
}

/***
 * CompositeRouter
 */

template < class RT, class CLA, class QM, class SCH >
CompositeRouter<RT,CLA,QM,SCH>::CompositeRouter(
		RT *rt, CLA *cla, QM *qm, SCH *sch, uint32_t n_ports)
	: m_rt(rt), m_cla(cla), m_qm(qm), m_sch(sch),
	  m_n_ports(n_ports)
{
	/* static check: make sure template parameters are of the correct classes */
	(void)static_cast<RoutingTable*>((RT*)0);
	(void)static_cast<Classifier*>((CLA*)0);
	(void)static_cast<QueueManager*>((QM*)0);
	(void)static_cast<Scheduler*>((SCH*)0);
}

template < class RT, class CLA, class QM, class SCH >
CompositeRouter<RT,CLA,QM,SCH>::~CompositeRouter() {}

/** helper for common functionality in push and push_batch */
template <class RT, class CLA, class QM>
inline  __attribute__((always_inline))
void composite_push(RT *rt, CLA *cla, QM *qm, struct emu_packet *packet,
		uint64_t cur_time)
{
	uint32_t port = rt->route(packet);
	uint32_t queue = cla->classify(packet, port);
	qm->enqueue(packet, port, queue, cur_time);
}

template < class RT, class CLA, class QM, class SCH >
void CompositeRouter<RT,CLA,QM,SCH>::push(struct emu_packet *packet,
		uint64_t cur_time)
	{ composite_push<RT,CLA,QM>(m_rt, m_cla, m_qm, packet, cur_time); }

template < class RT, class CLA, class QM, class SCH >
struct emu_packet *CompositeRouter<RT,CLA,QM,SCH>::pull(uint16_t port,
		uint64_t cur_time)
{
	return m_sch->schedule(port, cur_time);
}

template < class RT, class CLA, class QM, class SCH >
void CompositeRouter<RT,CLA,QM,SCH>::push_batch(struct emu_packet **pkts,
		uint32_t n_pkts, uint64_t cur_time)
{
	for (uint32_t i = 0; i < n_pkts; i++)
		composite_push<RT,CLA,QM>(m_rt, m_cla, m_qm, pkts[i], cur_time);
}

template < class RT, class CLA, class QM, class SCH >
uint32_t CompositeRouter<RT,CLA,QM,SCH>::pull_batch(struct emu_packet **pkts,
		uint32_t n_pkts, uint64_t *port_masks, uint64_t cur_time)
{ return composite_pull_batch<SCH>(m_sch, m_n_ports, pkts, n_pkts, port_masks,
		cur_time); }


/***
 * CompositeEndpointGroup
 */

template<class CLA, class QM, class SCH, class SINK>
inline CompositeEndpointGroup<CLA, QM, SCH, SINK>::CompositeEndpointGroup(
		CLA* cla, QM* qm, SCH* sch, SINK* sink, uint32_t first_endpoint_id,
		uint32_t n_endpoints)
	: m_cla(cla), m_qm(qm), m_sch(sch), m_sink(sink),
	  m_first_endpoint_id(first_endpoint_id), m_n_endpoints(n_endpoints)
{
	/* static check: make sure template parameters are of the correct classes */
	(void)static_cast<Classifier*>((CLA*)0);
	(void)static_cast<QueueManager*>((QM*)0);
	(void)static_cast<Scheduler*>((SCH*)0);
	(void)static_cast<Sink*>((SINK*)0);
}

template<class CLA, class QM, class SCH, class SINK>
inline CompositeEndpointGroup<CLA, QM, SCH, SINK>::~CompositeEndpointGroup() {}

template<class CLA, class QM, class SCH, class SINK>
inline void CompositeEndpointGroup<CLA, QM, SCH, SINK>::new_packets(
		struct emu_packet** pkts, uint32_t n_pkts, uint64_t cur_time)
{
	for (uint32_t i = 0; i < n_pkts; i++) {
		uint32_t port = pkts[i]->src - m_first_endpoint_id;
		uint32_t queue = m_cla->classify(pkts[i], port);
		m_qm->enqueue(pkts[i], port, queue, cur_time);
	}
}

template<class CLA, class QM, class SCH, class SINK>
inline void CompositeEndpointGroup<CLA, QM, SCH, SINK>::push_batch(
		struct emu_packet** pkts, uint32_t n_pkts)
{
	for (uint32_t i = 0; i < n_pkts; i++) {
		m_sink->handle(pkts[i]);
	}
}

template<class CLA, class QM, class SCH, class SINK>
inline uint32_t CompositeEndpointGroup<CLA, QM, SCH, SINK>::pull_batch(
		struct emu_packet** pkts, uint32_t n_pkts, uint64_t cur_time)
{
	uint64_t mask = 0xFFFFFFFFFFFFFFFF;
	return composite_pull_batch<SCH>(m_sch, m_n_endpoints, pkts, n_pkts, &mask,
			cur_time);
}

#endif /* COMPOSITE_H_ */
