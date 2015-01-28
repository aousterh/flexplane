/*
 * TorRoutingTable.h
 *
 *  Created on: Dec 17, 2014
 *      Author: yonch
 */

#ifndef ROUTINGTABLES_TOR_H_
#define ROUTINGTABLES_TOR_H_

#include <stdint.h>
#include "../composite.h"

/**
 * Packets flowing:
 *   - to endpoints in the rack are routed to the corresponding port
 *   - to other racks are routed to one of n_core routers using	an ECMP-like
 *   		scheme.
 */
class TorRoutingTable : public RoutingTable {
public:
	/**
	 * c'tor
	 * @param rack_shift: the number of bits to shift an endpoint ID to get
	 *      the index of each rack. The lower bits are the endpoint within the
	 *      rack.
	 * @param rack_index: the index of this rack
	 * @param n_endpoints: the actual number of endpoints in the rack, must be
	 *      no larger than (1 << log_max_rack_endpoints).
	 * @param n_uplinks: the number of uplinks to core routers
	 *
	 * @assumes endpoints are connected to ports 0..n_endpoints-1, and core
	 *     routers to ports n_endpoints..2*n_endpoints-1
	 */
	TorRoutingTable(uint32_t rack_shift, uint32_t rack_index,
			uint32_t n_endpoints, uint32_t n_uplinks);

	/**
	 * d'tor
	 */
	virtual ~TorRoutingTable();

	inline uint32_t route(struct emu_packet *pkt);

private:
	/** number of bits to shift endpoint IDs to get the rack */
	uint32_t m_rack_shift;

	/** mask to get the endpoint index within the rack */
	uint16_t m_endpoint_mask;

	/** index of the tor router's rack */
	uint32_t m_rack_index;

	/** number of endpoints in the current rack */
	uint32_t m_n_endpoints;

	/** the number of uplinks to cores */
	uint32_t m_n_uplinks;
};

inline TorRoutingTable::TorRoutingTable(uint32_t rack_shift,
		uint32_t rack_index, uint32_t n_endpoints, uint32_t n_uplinks)
	: m_rack_shift(rack_shift),
	  m_endpoint_mask( (1 << rack_shift) - 1 ),
	  m_rack_index(rack_index),
	  m_n_endpoints(n_endpoints),
	  m_n_uplinks(n_uplinks)
{}

inline TorRoutingTable::~TorRoutingTable() {}

inline uint32_t TorRoutingTable::route(struct emu_packet *pkt)
{
	uint16_t rack_id = (pkt->dst >> m_rack_shift);

	/* route within the rack? */
	if (rack_id == m_rack_index)
		return (pkt->dst & m_endpoint_mask);

	/* go to core switch - assume full bisection bandwidth, with multiple links
	 * used to create links with higher bandwidth */
	uint32_t hash =  7 * pkt->src + 9 * pkt->dst + pkt->flow;
	return (hash % m_n_uplinks) + m_n_endpoints;
}

#endif /* ROUTINGTABLES_TOR_H_ */
