/*
 * router.cc
 *
 *  Created on: December 19, 2014
 *      Author: aousterh
 */

#include "router.h"
#include "queue_managers/drop_tail.h"
#include "queue_managers/red.h"
#include <assert.h>
#include "output.h"

Router *RouterFactory::NewRouter(enum RouterType type, void *args,
		uint16_t id, Dropper &dropper) {
	switch (type) {
	case (R_DropTail):
            return new DropTailRouter((args == NULL ? 128 : ((struct drop_tail_args *)args)->q_capacity),
                                      dropper);
            
        case (R_RED):
            assert(args != NULL);
            return new REDRouter(id, (struct red_args *)args, dropper);                           
	}

	throw std::runtime_error("invalid router type\n");
}
