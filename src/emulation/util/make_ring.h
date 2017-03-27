/*
 * make_ring.h
 *
 *  Created on: Dec 21, 2014
 *      Author: yonch
 */

#ifndef UTIL_MAKE_RING_H_
#define UTIL_MAKE_RING_H_

#include "../graph-algo/fp_ring.h"
#include <stdexcept>
#include <string>

static inline
struct fp_ring *make_ring(const char *name, unsigned count, int socket_id,
		unsigned flags)
{
	/* Try to allocate the ring */
	struct fp_ring *ring = fp_ring_create(name, count, socket_id, flags);

	if (ring == NULL) {
		std::string msg(std::string("Could not allocate ring name='") + name + "'");
		throw std::runtime_error(msg);
	}

	return ring;
}

#endif /* UTIL_MAKE_RING_H_ */
