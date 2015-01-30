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
#include "queue_managers/hull.h"
#include <assert.h>
#include "output.h"

Router *RouterFactory::NewRouter(enum RouterType type, void *args,
		struct topology_args *topo_args, uint16_t id,
		struct queue_bank_stats *stats)
{
	struct drop_tail_args *dt_args;
	switch (type) {
	case (R_DropTail):
		assert(args != NULL);
		dt_args = (struct drop_tail_args *) args;
		if (topo_args->func == TOR_ROUTER)
			return new DropTailRouter(dt_args->q_capacity, stats,
					topo_args->rack_index);
		else
			return new DropTailCoreRouter(dt_args->q_capacity, stats,
					topo_args->links_per_tor);
	case (R_RED):
		assert(args != NULL);
		return new REDRouter(id, (struct red_args *)args, stats);

	case (R_DCTCP):
		assert(args != NULL);
		return new DCTCPRouter(id, (struct dctcp_args *)args, stats);

	case (R_Prio):
		assert(args == NULL);
		return new PriorityRouter(512, stats, topo_args->rack_index, 8, 8);

	case (R_RR):
		assert(args == NULL);
		return new RRRouter(512, stats, topo_args->rack_index);

	case (R_HULL):
		assert(args != NULL);
		return new HULLRouter(id, (struct hull_args *)args, stats);

	}

	throw std::runtime_error("invalid router type\n");
}
