/*
 * PyRoutingTable.h
 *
 *  Created on: Dec 25, 2014
 *      Author: yonch
 */

#ifndef ROUTING_TABLES_PYROUTINGTABLE_H_
#define ROUTING_TABLES_PYROUTINGTABLE_H_

#include "../composite.h"
#include <stdexcept>

class PyRoutingTable: public RoutingTable {
public:
	PyRoutingTable() {}
	virtual ~PyRoutingTable() {}

	virtual uint32_t route(struct emu_packet *pkt)
	{
		throw std::runtime_error("not implemented");
	}
};

#endif /* ROUTING_TABLES_PYROUTINGTABLE_H_ */
