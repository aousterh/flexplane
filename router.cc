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
#include "schedulers/hull_sched.h"
#include <assert.h>
#include "output.h"

Router *RouterFactory::NewRouter(enum RouterType type, void *args,
		enum RouterFunction func, uint32_t router_index,
		struct emu_topo_config *topo_config)
{
	struct drop_tail_args *dt_args;
	struct prio_by_src_args *prio_args;

	switch (type) {
	case (R_DropTail):
		assert(args != NULL);
		dt_args = (struct drop_tail_args *) args;
		if (func == TOR_ROUTER)
			return new DropTailRouter(dt_args->q_capacity, router_index,
					topo_config);
		else
			return new DropTailCoreRouter(dt_args->q_capacity, topo_config);
	case (R_RED):
		assert(args != NULL);
		return new REDRouter((struct red_args *)args, router_index,
				topo_config);

	case (R_DCTCP):
		assert(args != NULL);
		return new DCTCPRouter((struct dctcp_args *)args, router_index,
				topo_config);

	case (R_Prio):
		assert(args != NULL);
		prio_args = (struct prio_by_src_args *) args;
		return new PriorityBySourceRouter(prio_args, router_index,
				topo_config);

	case (R_Prio_by_flow):
		assert(args != NULL);
		dt_args = (struct drop_tail_args *) args;
		return new PriorityByFlowRouter(dt_args->q_capacity, router_index,
				topo_config);

	case (R_RR):
		assert(args != NULL);
		dt_args = (struct drop_tail_args *) args;
		return new RRRouter(dt_args->q_capacity, router_index, topo_config);

	case (R_HULL_sched):
		assert(args != NULL);
		return new HULLSchedRouter((struct hull_args *) args, router_index,
				topo_config);
	}

	throw std::runtime_error("invalid router type\n");
}
