/*
 * lstf_queue_bank.h
 *
 *  Created on: November 6, 2015
 *      Author: lut
 */

#ifndef LSTF_QUEUE_BANK_H_
#define LSTF_QUEUE_BANK_H_

#include "packet.h"
#include "queue_bank_log.h"
#include "../graph-algo/platform.h"
#include <climits>
#include <stdexcept>
#include <stdint.h>
#include <vector>

#define LSTF_MAX_SLACK 0xFFFFFFFFFFFFFFFF

/*
 * Metadata about a queued packet in LSTF.
 */
struct lstf_metadata {
        uint64_t slack;
        uint64_t arrival;
};


/**
 * A collection of LSTF queues. The queue bank keeps 1 queue for each of N
 * input ports. 
 */
class LSTFQueueBank {
public:
        /**
         * c'tor
         *
         * @param n_ports: the number of ports in the queue bank
         * @param queue_max_size: the maximum number of packets each queue can hold
         */
        inline LSTFQueueBank(uint32_t n_ports, uint32_t queue_max_size);

        /**
         * d'tor
         *
         * @assumes all memory pointed to by queues is already freed
         */
        inline ~LSTFQueueBank();

        /**
         * Enqueues the packet.
         * @param port: the port number to enqueue to
         * @param p: the packet to enqueue
         * @param cur_time: the current time
         */
        inline void enqueue(uint32_t port, struct emu_packet *p, uint64_t cur_time);

        /**
         * Dequeues the least-slack packet from this port.
         * @param port: the port number to dequeue from
         * @param cur_time: the current time
         * @assumes the queue is non-empty
         */
        inline struct emu_packet *dequeue_least_slack(uint32_t port, uint64_t cur_time);

        /**
         * Dequeues the highest-slack packet from this port.
         * @param port: the port number to dequeue from
         * @assumes the queue is non-empty
         */
        inline struct emu_packet *dequeue_most_slack(uint32_t port);

        /**
         * Return the slack of the highest-slack packet from this port.
         * @param port: the port number
         * @assumes the queue is non-empty
         */
        inline uint64_t most_slack(uint32_t port);

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

        /** a priority queue of packets for each port */
        std::vector<struct emu_packet **> m_queues;

        /** metadata array corresponding to m_queues */
        std::vector<struct lstf_metadata *> m_metadata;

        /** the occupancy of each port */
        std::vector<uint16_t> m_occupancies;

        /** a mask with 1 for non-empty ports, 0 for empty ports */
        uint64_t *m_non_empty_ports;

        /**logging stats */
        struct queue_bank_stats m_stats;

};

/** implementation */

inline LSTFQueueBank::LSTFQueueBank(uint32_t n_ports,
                uint32_t queue_max_size)
        : m_n_ports(n_ports),
          m_max_occupancy(queue_max_size)
{
        uint32_t i, pkt_pointers_size, metadata_array_size;
        
        /* initialize packet queues as empty */
        m_queues.reserve(n_ports);
        m_metadata.reserve(n_ports);
        
        /* calculate sizes */
        pkt_pointers_size = sizeof(struct emu_packet *) * queue_max_size;
        metadata_array_size = sizeof(struct lstf_metadata) * queue_max_size;

        /* initialize queue and metadata for every queue in the queue bank */
        for (i=0; i<n_ports; i++){
                /* allocate queue for packet pointers */
                struct emu_packet **pkt_pointers = (struct emu_packet **)
                        fp_malloc("LSTFPacketPointers", pkt_pointers_size);
                if (pkt_pointers == NULL)
                        throw std::runtime_error("could not allocate packet queue");
                m_queues.push_back(pkt_pointers);
                
                /* allocate metadata queue*/
                struct lstf_metadata *metadata_array = (struct lstf_metadata *)
                        fp_malloc("LSTFMetaData", metadata_array_size);
                m_metadata.push_back(metadata_array);
        }
        
        /* initialize occupancies to zero */
        m_occupancies.reserve(n_ports);
        for (i=0; i<n_ports; i++)
                m_occupancies.push_back(0);

        /* initialize port masks */
        uint32_t mask_n_64 = (n_ports + 63)/64;
        m_non_empty_ports = (uint64_t *)fp_calloc("non_empty_ports", 1,
                sizeof(uint64_t) * mask_n_64);
        if (m_non_empty_ports == NULL)
                throw std::runtime_error("could not allocate m_non_empty_ports");

        memset(&m_stats, 0, sizeof(m_stats));
}

inline LSTFQueueBank::~LSTFQueueBank()
{
        for (uint32_t i=0; i<m_n_ports; i++){
                free(m_queues[i]);
                free(m_metadata[i]);
        }

        free(m_non_empty_ports);
}

inline void LSTFQueueBank::enqueue(uint32_t port, struct emu_packet *p, uint64_t cur_time) {

        uint16_t index;

        /* mark port as non-empty */
        asm("bts %1,%0" : "+m" (*m_non_empty_ports) : "r" (port));

        /* put packet and metadata in first available spot */
        struct lstf_metadata *metadata = m_metadata[port];
        struct emu_packet **pkt_pointers = m_queues[port];

        index = m_occupancies[port];

        metadata[index].slack = p->slack;
        metadata[index].arrival = cur_time;

        pkt_pointers[index] = p;

        m_occupancies[port]++;

        queue_bank_log_enqueue(&m_stats, port);

}

inline struct emu_packet *LSTFQueueBank::dequeue_least_slack(
    uint32_t port, uint64_t cur_time) {
        uint16_t i, dequeue_index, last_index;
        uint64_t arrival;
        uint64_t tdiff;
        struct lstf_metadata least_slack;
        struct lstf_metadata *metadata;
        struct emu_packet *p;

        metadata = m_metadata[port];
        least_slack.slack = LSTF_MAX_SLACK;
        least_slack.arrival = 0;

        /* find the least-slack metadata */
        for (i=0; i<m_occupancies[port]; i++){

                if (metadata[i].slack + metadata[i].arrival <= least_slack.slack + least_slack.arrival){
                        memcpy(&least_slack, &metadata[i],
                                sizeof(struct lstf_metadata));
                        dequeue_index = i;
                }
        }

        p = m_queues[port][dequeue_index];
        arrival = metadata[dequeue_index].arrival;
        tdiff = cur_time - arrival;
        /* to prevent underflow */
        p->slack = tdiff <= p->slack ? p->slack - tdiff : 0;

        m_occupancies[port]--;

        /* copy last entry into vacated spot */
        last_index = m_occupancies[port];
        memcpy(&metadata[dequeue_index], &metadata[last_index],
                sizeof(struct lstf_metadata));
        m_queues[port][dequeue_index] = m_queues[port][last_index];

        uint64_t port_empty = (m_occupancies[port] == 0) & 0x1;
        m_non_empty_ports[port >> 6] ^= (port_empty << (port & 0x3F));

        queue_bank_log_dequeue(&m_stats, port);

        return p;
}

inline struct emu_packet *LSTFQueueBank::dequeue_most_slack(
    uint32_t port)
{
        /* functionality is very similar to dequeue_least_slack */
        uint16_t i, dequeue_index, last_index;
        struct lstf_metadata most_slack;
        struct lstf_metadata *metadata;
        struct emu_packet *p;

        metadata = m_metadata[port];
        most_slack.slack = 0;
        most_slack.arrival = 0;

        for(i=0; i<m_occupancies[port]; i++){
                if (metadata[i].slack + metadata[i].arrival >= most_slack.slack + most_slack.arrival){
                        memcpy(&most_slack, &metadata[i],
                                sizeof(struct lstf_metadata));
                        dequeue_index = i;
                }
        }
        
        p = m_queues[port][dequeue_index];
        
        m_occupancies[port]--;
        
        last_index = m_occupancies[port];
        memcpy(&metadata[dequeue_index], &metadata[last_index],
                sizeof(struct lstf_metadata));
        m_queues[port][dequeue_index] = m_queues[port][last_index];

        uint64_t port_empty = (m_occupancies[port] == 0) & 0x1;
        m_non_empty_ports[port >> 6] ^= (port_empty << (port & 0x3F));

        queue_bank_log_dequeue(&m_stats, port);

        return p;
}

inline uint64_t LSTFQueueBank::most_slack(uint32_t port){
        uint16_t i;
        uint64_t most_slack = 0;
        uint64_t cur_slack;

        struct lstf_metadata *metadata = m_metadata[port];

        for (i=0; i<m_occupancies[port]; i++){
                cur_slack = metadata[i].slack + metadata[i].arrival;
                if (cur_slack > most_slack)
                        most_slack = cur_slack;
        }

        return most_slack;
}

inline uint64_t *LSTFQueueBank::non_empty_port_mask(){
        return m_non_empty_ports;
}

inline int LSTFQueueBank::full(uint32_t port){
        return (m_occupancies[port] == m_max_occupancy);
}

inline int LSTFQueueBank::empty(uint32_t port){
        return (m_occupancies[port] == 0);
}

inline struct queue_bank_stats *LSTFQueueBank::get_queue_bank_stats(){
  return &m_stats;
}

#endif /* LSTF_QUEUE_BANK_H_ */
