/**
 * Code to support different platforms, e.g. DPDK v.s. vanilla gcc
 */

#ifndef GRAPH_ALGO_PLATFORM_H_
#define GRAPH_ALGO_PLATFORM_H_

#ifndef NO_DPDK

/** DPDK **/
#include <rte_malloc.h>
#include <rte_mempool.h>
#include <rte_branch_prediction.h>
#include <rte_errno.h>
#include <rte_prefetch.h>
#include "../protocol/platform/generic.h"
#include "../arbiter/dpdk-time.h"
#define fp_free(ptr)                            rte_free(ptr)
#define fp_calloc(typestr, num, size)           rte_calloc(typestr, num, size, 0)
#define fp_malloc(typestr, size)		rte_malloc(typestr, size, 0)
#define fp_pause()								rte_pause()

#define fp_mempool	 			rte_mempool
#define fp_mempool_get	 		rte_mempool_get
#define fp_mempool_put	 		rte_mempool_put
#define fp_mempool_get_bulk		rte_mempool_get_bulk
#define fp_mempool_put_bulk		rte_mempool_put_bulk

#define fp_prefetch0			rte_prefetch0

static struct fp_mempool * fp_mempool_create(const char *name, unsigned n,
		unsigned elt_size, unsigned cache_size, int socket_id, unsigned flags)
{
	return rte_mempool_create(name, n, elt_size, cache_size,
			0, NULL, NULL, NULL, NULL, socket_id, flags);
}

static const char *fp_strerror() {
	return rte_strerror(rte_errno);
}

#else

/** VANILLA **/
#include <errno.h>
#include <string.h>

#define fp_free(ptr)                            free(ptr)
#define fp_calloc(typestr, num, size)           calloc(num, size)
#define fp_malloc(typestr, size)		malloc(size)
#define fp_get_time_ns()				(1UL << 40)
#define fp_pause()						while (0) {}

#ifndef likely
#define likely(x)  __builtin_expect((x),1)
#endif /* likely */

#ifndef unlikely
#define unlikely(x)  __builtin_expect((x),0)
#endif /* unlikely */

/* do nothing for prefetch instructions */
#define fp_prefetch0

/* mempool */
struct fp_mempool {
	uint32_t total_elements;
	uint32_t cur_elements;
	void **elements;
};
static struct fp_mempool * fp_mempool_create(const char *name, unsigned n,
		unsigned elt_size, unsigned cache_size, int socket_id, unsigned flags)
{
	struct fp_mempool *mp;
	unsigned i;
	/* allocate the struct */
	mp = (struct fp_mempool *) malloc(sizeof(struct fp_mempool));
	if (mp == NULL)
		return NULL;
	/* populate the mempool struct */
	mp->total_elements = n;
	mp->cur_elements = 0;
	/* allocate the pointer table */
	mp->elements = (void **) malloc(n * sizeof(void *));
	if (mp->elements == NULL)
		goto cannot_alloc_ptrs;

	/* allocate structs */
	for (i = 0; i < n; i++) {
		mp->elements[i] = malloc(elt_size);
		if (mp->elements[i] == NULL)
			goto cannot_alloc_elements;
		mp->cur_elements++; /* cur_elements counts successfully alloc. elem. */
	}
	return mp;

cannot_alloc_elements:
	/* free all elements we successfully allocated */
	for (i = 0; i < mp->cur_elements; i++)
		free(mp->elements[i]);
	/* free the array itself */
	free(mp->elements);
cannot_alloc_ptrs:
	/* free the struct */
	free(mp);
	return NULL;
}
static inline int __attribute__((always_inline))
fp_mempool_get(struct fp_mempool *mp, void **obj_p) {
	if (unlikely(mp->cur_elements == 0))
		return -ENOENT;
	*obj_p = mp->elements[--(mp->cur_elements)];
	return 0;
}
static inline void __attribute__((always_inline))
fp_mempool_put(struct fp_mempool *mp, void *obj) {
	mp->elements[mp->cur_elements++] = obj;
}
static inline int __attribute__((always_inline))
fp_mempool_get_bulk(struct fp_mempool *mp, void **obj_table, unsigned n)
{
	if(unlikely(mp->cur_elements < n))
		return -ENOENT;
	mp->cur_elements -= n;
	memcpy(obj_table, &mp->elements[mp->cur_elements], n * sizeof(void *));
	return 0;
}
static inline void __attribute__((always_inline))
fp_mempool_put_bulk(struct fp_mempool *mp, void **obj_table, unsigned n)
{
	memcpy(&mp->elements[mp->cur_elements], obj_table, n * sizeof(void *));
	mp->cur_elements += n;
}
static const char *fp_strerror() {
	return("strerr not implemented");
}

#endif

#endif /* GRAPH_ALGO_PLATFORM_H_ */
