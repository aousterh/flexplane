
#ifndef UTIL_MAKE_MEMPOOL_H_
#define UTIL_MAKE_MEMPOOL_H_

#include "../graph-algo/platform.h"
#include <stdexcept>
#include <string>

static inline
struct fp_mempool *make_mempool(unsigned count, unsigned elt_size)
{
	/* Try to allocate the ring */
	struct fp_mempool *pool = fp_mempool_create(count, elt_size);

	if (pool == NULL) {
		std::string msg(std::string("Could not allocate mempool"));
		throw std::runtime_error(msg);
	}

	return pool;
}

#endif /* UTIL_MAKE_MEMPOOL_H_ */
