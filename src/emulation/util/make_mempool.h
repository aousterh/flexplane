
#ifndef UTIL_MAKE_MEMPOOL_H_
#define UTIL_MAKE_MEMPOOL_H_

#include "../graph-algo/platform.h"
#include <stdexcept>
#include <string>

static inline
struct fp_mempool *make_mempool(const char *name, unsigned n,
		unsigned elt_size, unsigned cache_size, int socket_id, unsigned flags)
{
	/* Try to allocate the ring */
	struct fp_mempool *pool = fp_mempool_create(name, n, elt_size, cache_size,
			socket_id, flags);

	if (pool == NULL) {
		std::string msg(std::string("Could not allocate mempool name=")
						+ std::string(name)
						+ std::string(" errno: ")
						+ fp_strerror());
		throw std::runtime_error(msg);
	}

	return pool;
}

#endif /* UTIL_MAKE_MEMPOOL_H_ */
