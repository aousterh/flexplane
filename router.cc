/*
 * router.cc
 *
 *  Created on: December 19, 2014
 *      Author: aousterh
 */

#include "router.h"
#include "drop_tail.h"
#include <assert.h>
#include "output.h"

Router *RouterFactory::NewRouter(enum RouterType type, void *args,
		uint16_t id, Dropper &dropper) {
	switch(type) {
	case(R_DropTail):
		assert(args != NULL);
		return new DropTailRouter(id, (struct drop_tail_args *) args,
				dropper);
	}
	throw std::runtime_error("invalid router type\n");
}
