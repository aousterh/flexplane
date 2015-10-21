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

#define PFABRIC_MAX_PRIORITY	0xFFFFFFFF

/* Metadata about a queued packet in pfabric, maintained in an array. On
 * enqueue, we add the new packet to the end of the array. On dequeue, we
 * iterate through the array and find the item to dequeue. We then copy the last
 * item in the array into the vacated spot, so that enqueue is constant time
 * and dequeue is linear in the number of queued packets. */
struct pfabric_metadata {
	uint16_t src;
	uint16_t dst;
	uint16_t id;
	uint32_t priority;
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

private:
	uint32_t m_n_ports;

	uint32_t m_max_occupancy;

	/** an array of packets for each port */
	std::vector<struct emu_packet **> m_queues;

	/** an array of packet metadata for each port (metadata corresponds to
	 * packets in m_packet_queues) */
	std::vector<struct pfabric_metadata *> m_metadata;

	/** the occupancy of each port */
	std::vector<uint16_t> m_occupancies;

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
	uint16_t i, j, pkt_pointers_size, metadata_array_size;

	/* initialize packet queues as empty */
	m_queues.reserve(n_ports);
	m_metadata.reserve(n_ports);

	/* calculate sizes
	 * TODO: round up to nearest multiple of 64 to avoid false sharing. */
	pkt_pointers_size = sizeof(struct emu_packet *) * queue_max_size;
	metadata_array_size = sizeof(struct pfabric_metadata) * queue_max_size;

	/* for every queue in the queue bank, initialize queue and metadata */
	for (i = 0; i < n_ports; i++) {
		/* allocate queue for packet pointers */
		struct emu_packet **pkt_pointers = (struct emu_packet **)
				fp_malloc("pFabricPacketPointers", pkt_pointers_size);
		if (pkt_pointers == NULL)
			throw std::runtime_error("could not allocate packet queue");
		m_queues.push_back(pkt_pointers);

		/* allocate metadata queue */
		struct pfabric_metadata *metadata_array = (struct pfabric_metadata *)
				fp_malloc("pFabricMetadata", metadata_array_size);
		m_metadata.push_back(metadata_array);
	}

	/* initialize occupancies to zero */
	m_occupancies.reserve(n_ports);
	for (i = 0; i < n_ports; i++)
		m_occupancies.push_back(0);

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
		free(m_queues[i]);
		free(m_metadata[i]);
	}

	free(m_non_empty_ports);
}

inline void PFabricQueueBank::enqueue(uint32_t port, struct emu_packet *p) {
	uint16_t index;

	/* mark port as non-empty */
	asm("bts %1,%0" : "+m" (*m_non_empty_ports) : "r" (port));

	/* put packet and metadata in first available spot */
	struct pfabric_metadata *metadata = m_metadata[port];
	struct emu_packet **pkt_pointers = m_queues[port];

        /* add to end of arrays */
	index = m_occupancies[port];
        metadata[index].src = p->src;
        metadata[index].dst = p->dst;
        metadata[index].id = p->id;
        metadata[index].priority = p->priority;
        pkt_pointers[index] = p;

	m_occupancies[port]++;

	queue_bank_log_enqueue(&m_stats, port);
}

inline struct emu_packet *PFabricQueueBank::dequeue_highest_priority(
		uint32_t port) {
	uint16_t i, dequeue_index, dequeue_id, last_index;
	struct pfabric_metadata highest_prio;
	struct pfabric_metadata *metadata;
	struct emu_packet *p;

	metadata = m_metadata[port];
	highest_prio.priority = PFABRIC_MAX_PRIORITY;

	/* find metadata with highest priority */
	for (i = 0; i < m_occupancies[port]; i++) {
		if (metadata[i].priority <= highest_prio.priority)
			memcpy(&highest_prio, &metadata[i],
					sizeof(struct pfabric_metadata));
	}

	/* find packet in this flow with lowest id in same flow, to avoid
	 * re-ordering. NOTE: flow should also be checked if we use flow with
	 * pFabric. */
	dequeue_id = highest_prio.id;
	for (i = 0; i < m_occupancies[port]; i++) {
		if ((metadata[i].src == highest_prio.src) &&
		    (metadata[i].dst == highest_prio.dst) &&
		    (metadata[i].id <= dequeue_id)) {
			dequeue_index = i;
			dequeue_id = metadata[i].id;
		}
	}

	p = m_queues[port][dequeue_index];

	/* bookkeeping */
	m_occupancies[port]--;

	/* copy last entry into vacated spot (changes nothing if this was in the
	   last spot) */
	last_index = m_occupancies[port];
	memcpy(&metadata[dequeue_index], &metadata[last_index],
	       sizeof(struct pfabric_metadata));
	m_queues[port][dequeue_index] = m_queues[port][last_index];

	uint64_t port_empty = (m_occupancies[port] == 0) & 0x1;
	m_non_empty_ports[port >> 6] ^= (port_empty << (port & 0x3F));

	queue_bank_log_dequeue(&m_stats, port);

	return p;
}

inline struct emu_packet *PFabricQueueBank::dequeue_lowest_priority(
		uint32_t port) {
	uint16_t i, last_index;
	struct pfabric_metadata *metadata = m_metadata[port];
	struct emu_packet *p;

	uint32_t lowest_priority = 0;
	uint16_t lowest_priority_index = 0;

	/* find entry with lowest priority (aka highest prio number) */
	for (i = 0; i < m_occupancies[port]; i++) {
		if (metadata[i].priority >= lowest_priority) {
			lowest_priority = metadata[i].priority;
			lowest_priority_index = i;
		}
	}

	p = m_queues[port][lowest_priority_index];

	/* bookkeeping */
	m_occupancies[port]--;

	/* copy last entry into vacated spot (changes nothing if this was in the
	   last spot) */
	last_index = m_occupancies[port];
	memcpy(&metadata[lowest_priority_index], &metadata[last_index],
	       sizeof(struct pfabric_metadata));
	m_queues[port][lowest_priority_index] = m_queues[port][last_index];

	uint64_t port_empty = (m_occupancies[port] == 0) & 0x1;
	m_non_empty_ports[port >> 6] ^= (port_empty << (port & 0x3F));

	queue_bank_log_dequeue(&m_stats, port);

	return p;
}

inline uint32_t PFabricQueueBank::lowest_priority(uint32_t port) {
	uint16_t i;
	uint32_t lowest_priority = 0;

	struct pfabric_metadata *metadata = m_metadata[port];
	for (i = 0; i < m_occupancies[port]; i++) {
		lowest_priority = MAX(lowest_priority, metadata[i].priority);
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

#endif /* PFABRIC_QUEUE_BANK_H_ */
