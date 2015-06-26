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

enum RouterType {
    R_DropTail, R_RED, R_DCTCP, R_Prio, R_RR, R_HULL, R_Prio_by_flow, R_HULL_sched
};

enum RouterFunction {
	TOR_ROUTER,
	CORE_ROUTER
};

struct topology_args {
	enum RouterFunction func;
    uint32_t rack_index;	/* tor router only */
    uint32_t links_per_tor; /* core router only */
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
	virtual void assign_to_core(Dropper *dropper,
			struct emu_admission_core_statistics *stat) = 0;
	virtual struct queue_bank_stats *get_queue_bank_stats() = 0;
	virtual struct port_drop_stats *get_port_drop_stats() = 0;
    virtual void push(struct emu_packet *packet,
    		uint64_t cur_time) = 0;
    virtual struct emu_packet *pull(uint16_t output, uint64_t cur_time) = 0;
    virtual void push_batch(struct emu_packet **pkts, uint32_t n_pkts,
    		uint64_t cur_time) = 0;
    virtual uint32_t pull_batch(struct emu_packet **pkts, uint32_t n_pkts,
    		uint64_t *port_masks, uint64_t cur_time) = 0;
};

/**
 * A class for constructing routers of different types.
 * @NewRouter: constructs a router with id @id of the specified type
 */
class RouterFactory {
public:
    static Router *NewRouter(enum RouterType type, void *args,
    		struct topology_args *topo_args, uint16_t id);
};
#endif

#endif /* ROUTER_H_ */
