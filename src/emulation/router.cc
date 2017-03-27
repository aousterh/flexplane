/*
 * router.cc
 *
 *  Created on: December 19, 2014
 *      Author: aousterh
 */

#include "router.h"
#include "queue_managers/drop_tail.h"
#include "queue_managers/drop_tail_tso.h"
#include "queue_managers/red.h"
#include "queue_managers/dctcp.h"
#include "queue_managers/pfabric_qm.h"
#include "queue_managers/lstf_qm.h"
#include "schedulers/hull_sched.h"
#include <assert.h>
#include "output.h"

Router *RouterFactory::NewRouter(enum RouterType type, void *args,
		enum RouterFunction func, uint32_t router_index,
		struct emu_topo_config *topo_config)
{
	struct drop_tail_args *dt_args;
	struct prio_by_src_args *prio_args;
	void *p_aligned; /* all memory must be aligned to 64-byte cache lines */

	switch (type) {
	case (R_DropTail):
		assert(args != NULL);
		dt_args = (struct drop_tail_args *) args;
		if (func == TOR_ROUTER) {
			p_aligned = fp_malloc("DropTailRouter",
					sizeof(class DropTailRouter));
			return new (p_aligned) DropTailRouter(dt_args->q_capacity,
					router_index, topo_config);
		} else {
			p_aligned = fp_malloc("DropTailCoreRouter",
					sizeof(class DropTailCoreRouter));
			return new (p_aligned) DropTailCoreRouter(dt_args->q_capacity,
					topo_config);
		}
	case (R_RED):
		assert(args != NULL);
		p_aligned = fp_malloc("REDRouter", sizeof(class REDRouter));
		return new (p_aligned) REDRouter((struct red_args *)args, router_index,
				topo_config);

	case (R_DCTCP):
		assert(args != NULL);
		p_aligned = fp_malloc("DCTCPRouter", sizeof(class DCTCPRouter));
		return new (p_aligned) DCTCPRouter((struct dctcp_args *)args,
				router_index, topo_config);

	case (R_Prio):
		assert(args != NULL);
		prio_args = (struct prio_by_src_args *) args;
		p_aligned = fp_malloc("PriorityBySourceRouter",
				sizeof(class PriorityBySourceRouter));
		return new (p_aligned) PriorityBySourceRouter(prio_args, router_index,
				topo_config);

	case (R_Prio_by_flow):
		assert(args != NULL);
		dt_args = (struct drop_tail_args *) args;
		p_aligned = fp_malloc("PriorityByFlowRouter",
				sizeof(class PriorityByFlowRouter));
		return new (p_aligned) PriorityByFlowRouter(dt_args->q_capacity,
				router_index, topo_config);

	case (R_RR):
		assert(args != NULL);
		dt_args = (struct drop_tail_args *) args;
		p_aligned = fp_malloc("RRRouter", sizeof(class RRRouter));
		return new (p_aligned) RRRouter(dt_args->q_capacity, router_index,
				topo_config);

	case (R_HULL_sched):
		assert(args != NULL);
		p_aligned = fp_malloc("HULLSchedRouter",
				sizeof(class HULLSchedRouter));
		return new (p_aligned) HULLSchedRouter((struct hull_args *) args,
				router_index, topo_config);

	case (R_PFabric):
		assert(args != NULL);
		p_aligned = fp_malloc("PFabricRouter", sizeof(class PFabricRouter));
		return new (p_aligned) PFabricRouter((struct pfabric_args *) args,
				router_index, topo_config);

	case (R_DropTailTSO):
		assert(args != NULL);
		dt_args = (struct drop_tail_args *) args;
		p_aligned = fp_malloc("DropTailTSORouter",
				sizeof(class DropTailTSORouter));
		return new (p_aligned) DropTailTSORouter(dt_args->q_capacity,
				router_index, topo_config);

        case (R_LSTF):
                assert(args != NULL);
                p_aligned = fp_malloc("LSTFRouter", sizeof(class LSTFRouter));
                return new (p_aligned) LSTFRouter((struct lstf_args *) args,
                                router_index, topo_config);
	}



	throw std::runtime_error("invalid router type\n");
}
