/*
 * pfabric_queue_bank.h
 *
 *  Created on: September 4, 2015
 *      Author: aousterh
 */

#ifndef PFABRIC_QUEUE_BANK_H_
#define PFABRIC_QUEUE_BANK_H_

#include "packet.h"
#include "queue_bank_log.h"
#include "../graph-algo/platform.h"
#include <climits>
#include <stdexcept>
#include <stdint.h>
#include <vector>

#define MAX(X, Y)	(((X) > (Y)) ? (X) : (Y))

#define PFABRIC_MAX_PRIORITY		0xFFFFFFFF
#define MAX_ACTIVE_FLOWS_PER_PORT	32

/* Metadata about a queued flow in pfabric. Head and tail point into an array
 * of packets. Within that array, packets in each flow are connected via a
 * linked list. Enqueue/dequeue scan the array of active flows, and then use
 * this metadata to point into the array of packets. TODO: also include flow in
 * this. */
struct pfabric_flow_metadata {
	uint16_t src;
	uint16_t dst;
	uint32_t head_priority;
	uint32_t tail_priority;
	struct pfabric_pkt_metadata *head; /* NULL if this is not in use */
	struct pfabric_pkt_metadata *tail;
};

/* Metadata about a queued packet and a pointer to the packet itself,
 * maintained in an array. On enqueue, we add the new packet to the end of the
 * array and set the next/prev pointers. On dequeue, we remove the pkt and copy
 * the last item in the array into the vacated spot, so that enqueue and
 * dequeue time are both constant in the number of queued packets. */
struct pfabric_pkt_metadata {
	struct emu_packet *pkt;
	uint32_t priority;
	struct pfabric_pkt_metadata *prev;
	struct pfabric_pkt_metadata *next;
	struct pfabric_flow_metadata *flow;
};

/**
 * A collection of PFabric queues. The queue bank keeps 1 queues for each of N
 * input ports
 */
class PFabricQueueBank {
public:
	/**
	 * c'tor
	 *
	 * @param n_ports: the number of ports in the queue bank
	 * @param queue_max_size: the maximum number of packets each queue can hold
	 *
	 * @important: this assumes exactly one queue per port
	 */
	inline PFabricQueueBank(uint32_t n_ports, uint32_t queue_max_size);

	/**
	 * d'tor
	 * @assumes all memory pointed to by queues is already freed
	 */
	inline ~PFabricQueueBank();

	/**
	 * Enqueues the packet
	 * @param port: the port number to enqueue to
	 * @param p: the packet to enqueue
	 * @assumes there is enough space in q
	 */
	inline void enqueue(uint32_t port, struct emu_packet *p);

	/**
	 * Dequeues the first packet from the flow of the highest priority packet,
	 * for this port.
	 * @param port: the port number to dequeue from
	 * @assumes the queue is non-empty
	 */
	inline struct emu_packet *dequeue_highest_priority(uint32_t port);

	/**
	 * Dequeues the lowest priority packet for this port.
	 * @param port: the port number to dequeue from
	 * @assumes the queue is non-empty
	 */
	inline struct emu_packet *dequeue_lowest_priority(uint32_t port);

	/**
	 * Return the priority of the lowest priority packet for this port.
	 * @assumes the queue is non-empty
	 */
	inline uint32_t lowest_priority(uint32_t port);

	/**
	 * @return a pointer to a bit mask with 1 for ports with packets, 0 o/w.
	 * @important user must not modify the bitmask contents
	 */
	inline uint64_t *non_empty_port_mask();

	/**
	 * @returns 1 if port is full, 0 otherwise
	 */
	inline int full(uint32_t port);

	/**
	 * @returns 1 if port is empty, 0 otherwise
	 */
	inline int empty(uint32_t port);

	/**
	 * @returns a pointer to the queue bank stats
	 */
	inline struct queue_bank_stats *get_queue_bank_stats();

	/**
	 * Debugging function which prints the contents of the queue bank.
	 */
	void print_contents();

private:
	/**
	 * Dequeues and returns the first packet from this port for this flow.
	 */
	struct emu_packet *dequeue_packet_from_flow(uint32_t port,
			struct pfabric_flow_metadata *flow_metadata);

	/**
	 * Removes a flow from the flow metadata array for this port
	 */
	void remove_flow(uint32_t port, struct pfabric_flow_metadata *flow);

	uint32_t m_n_ports;

	uint32_t m_max_occupancy;

	/** an array of flow metadata for each port (indexes into the
	 * corresponding packet metadata array) */
	std::vector<struct pfabric_flow_metadata *> m_flow_metadata;

	/** an array of packet metadata for each port */
	std::vector<struct pfabric_pkt_metadata *> m_pkt_metadata;

	/** the occupancy of each port */
	std::vector<uint16_t> m_occupancies;

	/** the number of flows for each port */
	std::vector<uint16_t> m_flows;

	/** a mask with 1 for non-empty ports, 0 for empty ports */
	uint64_t *m_non_empty_ports;

	/** logging stats */
	struct queue_bank_stats m_stats;
};

/** implementation */

inline PFabricQueueBank::PFabricQueueBank(uint32_t n_ports,
		uint32_t queue_max_size)
	: m_n_ports(n_ports),
	  m_max_occupancy(queue_max_size)
{
	uint16_t i, j, pkt_metadata_size, flow_metadata_size;

	/* initialize metadata queues as empty */
	m_pkt_metadata.reserve(n_ports);
	m_flow_metadata.reserve(n_ports);

	/* calculate sizes
	 * TODO: round up to nearest multiple of 64 to avoid false sharing. */
	pkt_metadata_size = sizeof(struct pfabric_pkt_metadata) * queue_max_size;
	flow_metadata_size = sizeof(struct pfabric_flow_metadata) *
			MAX_ACTIVE_FLOWS_PER_PORT;

	/* for every queue in the queue bank, initialize pkt and flow metadata */
	for (i = 0; i < n_ports; i++) {
		/* allocate queue for packet pointers */
		struct pfabric_pkt_metadata *pkt_metadata =
				(struct pfabric_pkt_metadata *)
				fp_malloc("pFabricPacketMetadata", pkt_metadata_size);
		if (pkt_metadata == NULL)
			throw std::runtime_error("could not allocate packet queue");
		m_pkt_metadata.push_back(pkt_metadata);

		/* allocate metadata queue */
		struct pfabric_flow_metadata *flow_metadata =
				(struct pfabric_flow_metadata *)
				fp_malloc("pFabricFlowMetadata", flow_metadata_size);
		for (j = 0; j < MAX_ACTIVE_FLOWS_PER_PORT; j++)
			flow_metadata[j].head = NULL;
		m_flow_metadata.push_back(flow_metadata);
	}

	/* initialize occupancies and flow counts to zero */
	m_occupancies.reserve(n_ports);
	m_flows.reserve(n_ports);
	for (i = 0; i < n_ports; i++) {
		m_occupancies.push_back(0);
		m_flows.push_back(0);
	}

	/* initialize port masks */
	uint32_t mask_n_64 = (n_ports + 63) / 64;
	m_non_empty_ports = (uint64_t *)fp_calloc("non_empty_ports", 1,
			sizeof(uint64_t) * mask_n_64);
	if (m_non_empty_ports == NULL)
		throw std::runtime_error("could not allocate m_non_empty_ports");

	memset(&m_stats, 0, sizeof(m_stats));
}

inline PFabricQueueBank::~PFabricQueueBank()
{
	for (uint32_t i = 0; i < m_n_ports; i++) {
		free(m_pkt_metadata[i]);
		free(m_flow_metadata[i]);
	}

	free(m_non_empty_ports);
}

inline void PFabricQueueBank::enqueue(uint32_t port, struct emu_packet *p) {
	uint16_t p_index;
	struct pfabric_flow_metadata *flow_metadata;

	/* mark port as non-empty */
	asm("bts %1,%0" : "+m" (*m_non_empty_ports) : "r" (port));

	/* put packet and metadata in first available spot */
	p_index = m_occupancies[port];
	struct pfabric_pkt_metadata *pkt_metadata = &m_pkt_metadata[port][p_index];
	pkt_metadata->pkt = p;
	pkt_metadata->priority = p->priority;
	pkt_metadata->next = NULL;

	/* find entry in flow queue */
	flow_metadata = m_flow_metadata[port];
	while (flow_metadata->head != NULL) {
		if (flow_metadata >= m_flow_metadata[port] + MAX_ACTIVE_FLOWS_PER_PORT)
			throw std::runtime_error("too many active flows");

		if ((flow_metadata->src == p->src) && (flow_metadata->dst == p->dst))
			goto found_flow_metadata;

		flow_metadata++;
	}
	/* no existing flow entry - initialize a new one */
	flow_metadata->src = p->src;
	flow_metadata->dst = p->dst;
	flow_metadata->head_priority = p->priority;
	flow_metadata->tail_priority = p->priority;
	flow_metadata->head = pkt_metadata;
	flow_metadata->tail = NULL;
	m_flows[port]++;

found_flow_metadata:
	/* add pkt metadata to linked list */
	pkt_metadata->prev = flow_metadata->tail;
	pkt_metadata->flow = flow_metadata;
	if (flow_metadata->tail != NULL)
		flow_metadata->tail->next = pkt_metadata;
	flow_metadata->tail = pkt_metadata;
	flow_metadata->tail_priority = p->priority;

	m_occupancies[port]++;

	queue_bank_log_enqueue(&m_stats, port);
}

inline struct emu_packet *PFabricQueueBank::dequeue_highest_priority(
		uint32_t port) {
	struct pfabric_flow_metadata *flow_metadata, *highest_pri_flow_metadata;
	uint32_t highest_priority = PFABRIC_MAX_PRIORITY;

	/* find the flow with the highest priority */
	flow_metadata = m_flow_metadata[port];
	while (flow_metadata->head != NULL) {
		if (flow_metadata->tail_priority <= highest_priority) {
			highest_pri_flow_metadata = flow_metadata;
			highest_priority = flow_metadata->tail_priority;
		}

		flow_metadata++;
	}

	return dequeue_packet_from_flow(port, highest_pri_flow_metadata);
}

inline struct emu_packet *PFabricQueueBank::dequeue_lowest_priority(
		uint32_t port) {
	struct pfabric_flow_metadata *flow_metadata, *lowest_pri_flow_metadata;
	uint32_t lowest_priority = 0;

	/* find the flow with the lowest priority */
	flow_metadata = m_flow_metadata[port];
	while (flow_metadata->head != NULL) {
		if (flow_metadata->tail_priority >= lowest_priority) {
			lowest_pri_flow_metadata = flow_metadata;
			lowest_priority = flow_metadata->tail_priority;
		}

		flow_metadata++;
	}

	return dequeue_packet_from_flow(port, lowest_pri_flow_metadata);
}

inline uint32_t PFabricQueueBank::lowest_priority(uint32_t port) {
	struct pfabric_flow_metadata *flow_metadata;
	uint32_t lowest_priority = 0;

	flow_metadata = m_flow_metadata[port];
	while (flow_metadata->head != NULL) {
		lowest_priority = MAX(lowest_priority, flow_metadata->head_priority);
		flow_metadata++;
	}

	return lowest_priority;
}

inline uint64_t *PFabricQueueBank::non_empty_port_mask()
{
	return m_non_empty_ports;
}

inline int PFabricQueueBank::full(uint32_t port)
{
	return (m_occupancies[port] == m_max_occupancy);
}

inline int PFabricQueueBank::empty(uint32_t port)
{
	return (m_occupancies[port] == 0);
}

inline struct queue_bank_stats *PFabricQueueBank::get_queue_bank_stats() {
	return &m_stats;
}

inline emu_packet *PFabricQueueBank::dequeue_packet_from_flow(uint32_t port,
		struct pfabric_flow_metadata *flow_metadata) {
	struct pfabric_pkt_metadata *pkt_metadata, *last_pkt_metadata;
	struct emu_packet *p;

	/* dequeue the first packet in this flow */
	pkt_metadata = flow_metadata->head;
	p = pkt_metadata->pkt;
	flow_metadata->head = pkt_metadata->next;
	if (pkt_metadata->next != NULL) {
		pkt_metadata->next->prev = NULL;
		flow_metadata->head_priority = flow_metadata->head->priority;
	}

	/* remove from flow_metadata queue if this flow now has no packets */
	if (pkt_metadata->next == NULL)
		remove_flow(port, flow_metadata);

	/* bookkeeping */
	m_occupancies[port]--;

	/* move the last entry in the pkt queue to the vacated spot (changes
	 * nothing if this was in the last spot), update linked-list pointers */
	last_pkt_metadata = &m_pkt_metadata[port][m_occupancies[port]];
	if (pkt_metadata != last_pkt_metadata) {
		memcpy(pkt_metadata, last_pkt_metadata,
				sizeof(struct pfabric_pkt_metadata));
		if (pkt_metadata->prev != NULL)
			pkt_metadata->prev->next = pkt_metadata;
		else
			pkt_metadata->flow->head = pkt_metadata;
		if (pkt_metadata->next != NULL)
			pkt_metadata->next->prev = pkt_metadata;
		else
			pkt_metadata->flow->tail = pkt_metadata;
	}

	/* mark port as empty if necessary */
	uint64_t port_empty = (m_occupancies[port] == 0) & 0x1;
	m_non_empty_ports[port >> 6] ^= (port_empty << (port & 0x3F));

	queue_bank_log_dequeue(&m_stats, port);
	return p;
}

inline void PFabricQueueBank::remove_flow(uint32_t port,
		struct pfabric_flow_metadata *flow) {
	struct pfabric_flow_metadata *last_flow;
	struct pfabric_pkt_metadata *pkt;

	m_flows[port]--;
	last_flow = &m_flow_metadata[port][m_flows[port]];

	if (last_flow != flow)
		memcpy(flow, last_flow, sizeof(struct pfabric_flow_metadata));

	last_flow->head = NULL;

	/* update pointers from packets in this flow */
	pkt = flow->head;
	while (pkt != NULL) {
		pkt->flow = flow;
		pkt = pkt->next;
	}
}

inline void PFabricQueueBank::print_contents() {
	uint32_t port;
	struct pfabric_flow_metadata *flow;
	struct pfabric_pkt_metadata *pkt;

	for (port = 0; port < m_n_ports; port++) {
		if (m_occupancies[port] > 0) {
			printf("port %d with %d flows:\n", port, m_flows[port]);
			flow = &m_flow_metadata[port][0];

			while (flow->head != NULL) {
				printf("flow from %d to %d with prios from %d to %d: ",
						flow->src, flow->dst, flow->head_priority,
						flow->tail_priority);
				pkt = flow->head;
				while (pkt != NULL) {
					printf("%d, ", pkt->priority);
					pkt = pkt->next;
				}
				printf("\n");

				flow++;
			}
		}
	}
}

#endif /* PFABRIC_QUEUE_BANK_H_ */
