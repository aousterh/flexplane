/*
 * TorClassifier.h
 *
 *  Created on: Dec 17, 2014
 *      Author: yonch
 */

#ifndef CLASSIFIERS_TORCLASSIFIER_H_
#define CLASSIFIERS_TORCLASSIFIER_H_

#include <stdint.h>
#include "../composite.h"

/**
 * Classifies packets flowing to endpoints in the rack to the corresponding
 * 		port, and packets to other rack to one of n_core core routers using
 * 		an ECMP-like scheme.
 *
 * The queue within the port is determined by the packet's flow ID.
 */
class TorClassifier : public Classifier {
public:
	/**
	 * c'tor
	 * @param rack_shift: the number of bits to shift an endpoint ID to get
	 *      the index of each rack. The lower bits are the endpoint within the
	 *      rack.
	 * @param rack_index: the index of this rack
	 * @param n_endpoints: the actual number of endpoints in the rack, must be
	 *      no larger than (1 << log_max_rack_endpoints).
	 * @param n_core: the number of core routers
	 *
	 * @assumes endpoints are connected to ports 0..n_endpoints-1, and core
	 *     routers to ports n_endpoints..n_endpoints+n_core-1
	 */
	TorClassifier(uint32_t rack_shift, uint32_t rack_index,
			uint32_t n_endpoints, uint32_t n_core);

	/**
	 * d'tor
	 */
	virtual ~TorClassifier();

	inline void classify(struct emu_packet *pkt, uint32_t *port, uint32_t *queue);

private:
	/** number of bits to shift endpoint IDs to get the rack */
	uint32_t m_rack_shift;

	/** mask to get the endpoint index within the rack */
	uint16_t m_endpoint_mask;

	/** index of the tor router's rack */
	uint32_t m_rack_index;

	/** number of endpoints in the current rack */
	uint32_t m_n_endpoints;

	/** the number of core routers */
	uint32_t m_n_core;
};

inline TorClassifier::TorClassifier(uint32_t rack_shift, uint32_t rack_index,
		uint32_t n_endpoints, uint32_t n_core)
	: m_rack_shift(rack_shift),
	  m_endpoint_mask( (1 << rack_shift) - 1 ),
	  m_rack_index(rack_index),
	  m_n_endpoints(n_endpoints),
	  m_n_core(n_core)
{}

inline TorClassifier::~TorClassifier() {}

inline void TorClassifier::classify(struct emu_packet* pkt, uint32_t* port,
		uint32_t* queue)
{
	uint16_t rack_id = (pkt->dst >> m_rack_shift);

	if (rack_id == m_rack_index) {
		/* route within the rack */
		*port = pkt->dst & m_endpoint_mask;
	} else {
		/* go to core switch */
		uint32_t hash =  7 * pkt->src + 9 * pkt->dst + pkt->flow;
		*port = hash % m_n_core;
	}

	*queue = pkt->flow;
}

#endif /* CLASSIFIERS_TORCLASSIFIER_H_ */
