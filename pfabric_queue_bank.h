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
#include <climits>
#include <stdexcept>
#include <stdint.h>
#include <vector>

#define MAX(X, Y)	(((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y)	(((X) < (Y)) ? (X) : (Y))

#define PFABRIC_QUEUE_CAPACITY	24
uint32_t PFABRIC_MAX_PRIORITY = ULONG_MAX;

/* Metadata about a queued packet in pfabric.
 * TODO: on dequeue, copy last entry to freed spot. This allows us to remove
 * the in_use field and only visit the number of entries used on each
 * operation. Then enqueue would also be constant time. */
struct pfabric_metadata {
	bool in_use;
	uint16_t src;
	uint16_t dst;
	uint16_t id;
	uint32_t priority;
};

/**
 * A collection of PFabric queues. The queue bank keeps 1 queues for each of N
 * input ports
 */
class PfabricQueueBank {
public:
	/**
	 * c'tor
	 *
	 * @param n_ports: the number of ports in the queue bank
	 * @param queue_max_size: the maximum number of packets each queue can hold
	 *
	 * @important: this assumes exactly one queue per port
	 */
	PfabricQueueBank(uint32_t n_ports, uint32_t queue_max_size);

	/**
	 * d'tor
	 * @assumes all memory pointed to by queues is already freed
	 */
	~PfabricQueueBank();

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
	 * @returns 1 if port is empty, 0 otherwise
	 */
	inline int full(uint32_t port);

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

PfabricQueueBank::PfabricQueueBank(uint32_t n_ports, uint32_t queue_max_size)
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
				malloc(pkt_pointers_size);
		if (pkt_pointers == NULL)
			throw std::runtime_error("could not allocate packet queue");
		m_queues.push_back(pkt_pointers);

		/* allocate metadata queue */
		struct pfabric_metadata *metadata_array = (struct pfabric_metadata *)
				malloc(metadata_array_size);
		m_metadata.push_back(metadata_array);

		for (j = 0; j < queue_max_size; j++)
			metadata_array[i].in_use = false;
	}

	/* initialize occupancies to zero */
	m_occupancies.reserve(n_ports);
	for (i = 0; i < n_ports; i++)
		m_occupancies.push_back(0);

	/* initialize port masks */
	uint32_t mask_n_64 = (n_ports + 63) / 64;
	m_non_empty_ports = (uint64_t *)calloc(1, sizeof(uint64_t) * mask_n_64);
	if (m_non_empty_ports == NULL)
		throw std::runtime_error("could not allocate m_non_empty_ports");

	memset(&m_stats, 0, sizeof(m_stats));
}

PfabricQueueBank::~PfabricQueueBank()
{
	for (uint32_t i = 0; i < m_n_ports; i++) {
		free(m_queues[i]);
		free(m_metadata[i]);
	}

	free(m_non_empty_ports);
}

inline void PfabricQueueBank::enqueue(uint32_t port, struct emu_packet *p) {
	uint16_t i;

	/* mark port as non-empty */
	asm("bts %1,%0" : "+m" (*m_non_empty_ports) : "r" (port));

	/* put packet and metadata in first available spot */
	struct pfabric_metadata *metadata = m_metadata[port];
	struct emu_packet **pkt_pointers = m_queues[port];
	for (i = 0; i < m_max_occupancy; i++) {
		if (!metadata[i].in_use) {
			/* found an available spot, fill it */
			metadata[i].in_use = true;
			metadata[i].src = p->src;
			metadata[i].dst = p->dst;
			metadata[i].id = p->id;
			metadata[i].priority = p->priority;
			pkt_pointers[i] = p;

			break;
		}
	}

	m_occupancies[port]++;

	queue_bank_log_enqueue(&m_stats, port);
}

inline struct emu_packet *PfabricQueueBank::dequeue_highest_priority(
		uint32_t port) {
	uint16_t i, dequeue_index, dequeue_id;
	struct pfabric_metadata highest_prio;
	struct pfabric_metadata *metadata;

	metadata = m_metadata[port];
	highest_prio.priority = PFABRIC_MAX_PRIORITY;

	/* find metadata with highest priority */
	for (i = 0; i < m_max_occupancy; i++) {
		if (metadata[i].in_use &&
				(metadata[i].priority < highest_prio.priority))
			memcpy(&highest_prio, &metadata[i],
					sizeof(struct pfabric_metadata));
	}

	/* find packet in this flow with lowest id in same flow, to avoid
	 * re-ordering. NOTE: flow should also be checked if we use flow with
	 * pFabric. */
	dequeue_id = highest_prio.id;
	for (i = 0; i < m_max_occupancy; i++) {
		if (metadata[i].in_use && (metadata[i].src == highest_prio.src) &&
				(metadata[i].dst == highest_prio.dst) &&
				(metadata[i].id <= dequeue_id)) {
			dequeue_index = i;
			dequeue_id = metadata[i].id;
		}
	}

	/* bookkeeping */
	metadata[dequeue_index].in_use = false;
	m_occupancies[port]--;

	uint64_t port_empty = (m_occupancies[port] == 0) & 0x1;
	m_non_empty_ports[port >> 6] ^= (port_empty << (port & 0x3F));

	queue_bank_log_dequeue(&m_stats, port);

	return m_queues[port][dequeue_index];
}

inline struct emu_packet *PfabricQueueBank::dequeue_lowest_priority(
		uint32_t port) {
	uint16_t i;
	struct pfabric_metadata *metadata = m_metadata[port];

	uint32_t lowest_priority = 0;
	uint16_t lowest_priority_index = 0;

	/* find entry with lowest priority (aka highest prio number) */
	for (i = 0; i < m_max_occupancy; i++) {
		if (metadata[i].in_use && (metadata[i].priority > lowest_priority)) {
			lowest_priority = metadata[i].priority;
			lowest_priority_index = i;
		}
	}

	/* bookkeeping */
	metadata[lowest_priority_index].in_use = false;
	m_occupancies[port]--;

	uint64_t port_empty = (m_occupancies[port] == 0) & 0x1;
	m_non_empty_ports[port >> 6] ^= (port_empty << (port & 0x3F));

	queue_bank_log_dequeue(&m_stats, port);

	return m_queues[port][lowest_priority_index];
}

inline uint32_t PfabricQueueBank::lowest_priority(uint32_t port) {
	uint16_t i;
	uint32_t lowest_priority = 0;

	struct pfabric_metadata *metadata = m_metadata[port];
	for (i = 0; i < m_max_occupancy; i++) {
		if (metadata[i].in_use)
			lowest_priority = MAX(lowest_priority, metadata[i].priority);
	}

	return lowest_priority;
}

inline uint64_t *PfabricQueueBank::non_empty_port_mask()
{
	return m_non_empty_ports;
}

inline int PfabricQueueBank::full(uint32_t port)
{
	return (m_occupancies[port] == m_max_occupancy);
}

inline struct queue_bank_stats *PfabricQueueBank::get_queue_bank_stats() {
	return &m_stats;
}

#endif /* PFABRIC_QUEUE_BANK_H_ */
