
#include "pfabric.h"
#include <stdbool.h>

/*
 * In this file (in order):
 *  - Data structures
 *  - Utility functions impl
 *  - Router impl
 *  - Endpoint impl
 *  - router_ops and endpoint_ops
 */

/***************************************
 * DATA STRUCTURES
 ***************************************/

/**
 * A queue for a flow (source,destination) pair
 */
struct pfabric_per_flow_queue
{
	/* queue of packets */
	struct emu_packet *q[PFABRIC_MAX_QUEUE_LEN];

	/* indices into q: tail is the tail's index + 1; head is the head's index */
	uint32_t head;
	uint32_t tail;

	/* member of priority radix-heap */
	struct list_node prio_heap;
};

/**
 * A structure kept per pfabric port
 */
struct pfabric_per_port
{
	/* a separate queue per flow */
	struct pfabric_per_flow_queue flows[PFABRIC_MAX_ENDPOINTS];

	/* keep active flows in a priority queue */
	struct radix_heap prio;
};

/**
 * The per router private struct
 */
struct pfabric_router_priv {
	/* keep the priority queue per port */
	struct pfabric_per_port ports[PFABRIC_MAX_ROUTER_PORTS];
};

struct pfabric_endpoint_priv {
	struct pfabric_per_port port;
};

/***************************************
 * UTILITY FUNCS
 ***************************************/
static inline void pfq_init(struct pfabric_per_flow_queue *pfq) {
	pfq->head = pfq->tail = 0;
}
static inline bool pfq_is_empty(struct pfabric_per_flow_queue *pfq) {
	return pfq->head == pfq->tail;
}
static inline bool pfq_is_full(struct pfabric_per_flow_queue *pfq) {
	return pfq->tail - pfq->head == PFABRIC_MAX_QUEUE_LEN;
}
static inline void pfq_enq(struct pfabric_per_flow_queue *pfq,
		struct emu_packet *pkt)
{
	pfq->q[(pfq->tail++) % PFABRIC_MAX_QUEUE_LEN] = pkt;
}
static inline struct emu_packet *pfq_deq(struct pfabric_per_flow_queue *pfq) {
	return pfq->q[(pfq->head++) % PFABRIC_MAX_QUEUE_LEN];
}

static inline void pf_port_init(struct pfabric_per_port *pf_port) {
	int i;
	rheap_init(&pf_port->prio);
	for (i = 0; i < PFABRIC_MAX_ENDPOINTS; i++)
		pfq_init(&pf_port->flows[i]);
}

/***************************************
 * ROUTER
 ***************************************/
static int pfabric_rtr_init(struct emu_router *rtr, void * args)
{
	struct pfabric_router_priv *priv = router_priv(rtr);
	int i;

	for (i = 0; i < PFABRIC_MAX_ROUTER_PORTS; i++)
		pf_port_init(&priv->ports[i]);

	return 0;
}

static void *pfabric_rtr_cleanup(struct emu_router *rtr)
{

}

static void pfabric_rtr_emulate(struct emu_router *rtr)
{

}

/***************************************
 * ENDPOINT
 ***************************************/
int pfabric_ep_init(struct emu_endpoint *ep, void *args)
{

}

void pfabric_ep_reset(struct emu_endpoint *ep)
{

}

void pfabric_ep_cleanup(struct emu_endpoint *ep)
{

}

void pfabric_ep_emulate(struct emu_endpoint *ep)
{

}

/***************************************
 * OPS STRUCTS
 ***************************************/

struct router_ops pfabric_router_ops = {
	.priv_size = sizeof(struct pfabric_router_priv),
	.init = &pfabric_rtr_init,
	.cleanup = &pfabric_rtr_cleanup,
	.emulate = &pfabric_rtr_emulate
};

struct endpoint_ops pfabric_endpoint_ops = {
	.priv_size = sizeof(struct pfabric endpoint_priv),
};
