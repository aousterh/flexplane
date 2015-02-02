/*
 * CoreRoutingTable.h
 *
 *  Created on: January 27, 2014
 *      Author: aousterh
 */

#ifndef ROUTINGTABLES_CORE_H_
#define ROUTINGTABLES_CORE_H_

#include <stdint.h>
#include "../composite.h"

/**
 * Packets are routed to the ToR corresponding to their destination. We assume
 * full bisection bandwidth.
 */
class CoreRoutingTable : public RoutingTable {
public:
	/**
	 * c'tor
	 * @param tor_mask: mask for number of links per tor. we assume this is the
	 * 	negation of the mask to get the rack id
	 *
	 * @assumes tors are connected to ports tor*links_per_tor...
	 * 	(tor+1)*links_per_tor-1
	 */
	CoreRoutingTable(uint32_t tor_mask);

	/**
	 * d'tor
	 */
	virtual ~CoreRoutingTable();

	inline uint32_t route(struct emu_packet *pkt);

private:
	/** mask for links to tor */
	uint32_t m_tor_mask;
};

inline CoreRoutingTable::CoreRoutingTable(uint32_t tor_mask)
	: m_tor_mask(tor_mask)
{}

inline CoreRoutingTable::~CoreRoutingTable() {}

inline uint32_t CoreRoutingTable::route(struct emu_packet *pkt)
{
	/* use a hash to choose between the links to the correct tor */
	uint32_t hash =  7 * pkt->src + 9 * pkt->dst + pkt->flow;
#if defined(TWO_RACK_TOPOLOGY)
	return (hash & m_tor_mask) + (pkt->dst & ~m_tor_mask);
#else
	/* must be 3 rack topology */
	return (hash & 0xF) + ((pkt->dst & ~m_tor_mask) >> 1);
#endif
}

#endif /* ROUTINGTABLES_CORE_H_ */
