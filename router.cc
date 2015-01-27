/*
 * router.cc
 *
 *  Created on: December 19, 2014
 *      Author: aousterh
 */

#include "router.h"
#include "queue_managers/drop_tail.h"
#include "queue_managers/red.h"
#include "queue_managers/dctcp.h"
#include <assert.h>
#include "output.h"

Router *RouterFactory::NewRouter(enum RouterType type, void *args, uint16_t id,
		struct queue_bank_stats *stats)
{
  uint16_t q_capacity;
	switch (type) {
	case (R_DropTail):
          q_capacity =   (args == NULL)
                       ? 128
		       : ((struct drop_tail_args *)args)->q_capacity;
		return new DropTailRouter(q_capacity, stats);

	case (R_RED):
		assert(args != NULL);
		return new REDRouter(id, (struct red_args *)args, stats);

	case (R_DCTCP):
		assert(args != NULL);
		return new DCTCPRouter(id, (struct dctcp_args *)args, stats);
	}

	throw std::runtime_error("invalid router type\n");
}
