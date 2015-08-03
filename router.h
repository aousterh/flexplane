/*
 * router.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include <inttypes.h>

struct emu_packet;
struct emu_topo_config;

enum RouterType {
    R_DropTail, R_RED, R_DCTCP, R_Prio, R_RR, R_Prio_by_flow, R_HULL_sched
};

enum RouterFunction {
	TOR_ROUTER,
	CORE_ROUTER
};

#ifdef __cplusplus
class Dropper;
/**
 * A representation of a router in the emulated network.
 * @push: enqueue a single packet to this router
 * @pull: dequeue a single packet from port output at this router
 * @push_batch: enqueue a batch of several packets to this router
 * @pull_batch: dequeue a batch of several packets from this router
 */
class Router {
public:
    Router() {};
    virtual ~Router() {};
	virtual struct queue_bank_stats *get_queue_bank_stats() = 0;
    virtual void push(struct emu_packet *packet,
    		uint64_t cur_time, Dropper *dropper) = 0;
    virtual struct emu_packet *pull(uint16_t output, uint64_t cur_time,
    		Dropper *dropper) = 0;
    virtual void push_batch(struct emu_packet **pkts, uint32_t n_pkts,
    		uint64_t cur_time, Dropper *dropper) = 0;
    virtual uint32_t pull_batch(struct emu_packet **pkts, uint32_t n_pkts,
    		uint64_t *port_masks, uint64_t cur_time, Dropper *dropper) = 0;
};

/**
 * A class for constructing routers of different types.
 * @NewRouter: constructs a router of the specified type. func specifies
 * 	whether it is a Core or ToR. router_index is the router number, and
 * 	also the rack_index for ToRs.
 */
class RouterFactory {
public:
    static Router *NewRouter(enum RouterType type, void *args,
    		enum RouterFunction func, uint32_t router_index,
    		struct emu_topo_config *topo_config);
};
#endif

#endif /* ROUTER_H_ */
