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
	 * @param rack_shift: the number of bits to shift an endpoint ID to get
	 *      the index of each rack. The lower bits are the endpoint within the
	 *      rack.
	 * @param links_per_tor: number of links to each tor switch
	 *
	 * @assumes tors are connected to ports tor*links_per_tor...
	 * 	(tor+1)*links_per_tor-1
	 */
	CoreRoutingTable(uint32_t rack_shift, uint32_t links_per_tor);

	/**
	 * d'tor
	 */
	virtual ~CoreRoutingTable();

	inline uint32_t route(struct emu_packet *pkt);

private:
	/** number of bits to shift endpoint IDs to get the rack */
	uint32_t m_rack_shift;

	/** links per tor router */
	uint32_t m_links_per_tor;
};

inline CoreRoutingTable::CoreRoutingTable(uint32_t rack_shift,
		uint32_t links_per_tor)
	: m_rack_shift(rack_shift),
	  m_links_per_tor(links_per_tor)
{}

inline CoreRoutingTable::~CoreRoutingTable() {}

inline uint32_t CoreRoutingTable::route(struct emu_packet *pkt)
{
	uint16_t rack_id = (pkt->dst >> m_rack_shift);

	/* use a hash to choose between the links to the correct tor */
	uint32_t hash =  7 * pkt->src + 9 * pkt->dst + pkt->flow;
	return (hash % m_links_per_tor) + rack_id * m_links_per_tor;
}

#endif /* ROUTINGTABLES_CORE_H_ */
