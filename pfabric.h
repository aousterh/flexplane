/*
 * pfabric.h
 *
 *  Created on: Aug 31, 2014
 *      Author: yonch
 */

#ifndef PFABRIC_H_
#define PFABRIC_H_

#include "api.h"

#define PFABRIC_MAX_ENDPOINTS		64

/* Pfabric's 18KB queues -> ~13 MTUs */
#define PFABRIC_MAX_QUEUE_LEN		16

#define PFABRIC_MAX_ROUTER_PORTS	32


extern struct router_ops pfabric_router_ops;



#endif /* PFABRIC_H_ */
