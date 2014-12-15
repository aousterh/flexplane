/*
 * emulation.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef EMULATION_H_
#define EMULATION_H_

#include "config.h"
#include "api.h"
#include "endpoint.h"
#include "endpoint_group.h"
#include "router.h"
#include "admissible_log.h"
#include "../graph-algo/fp_ring.h"
#include "../graph-algo/platform.h"

#include <inttypes.h>

#define ADMITTED_MEMPOOL_SIZE	10
#define ADMITTED_Q_LOG_SIZE		4
#define PACKET_MEMPOOL_SIZE		(1024 * 128)
#define PACKET_Q_LOG_SIZE		12
#define EMU_NUM_PACKET_QS		(2 + EMU_NUM_ROUTERS)

class Emulation;

extern Emulation *g_state;

/**
 * Data structure to store the state of the emulation.
 * @add_backlog: add amount of demand from src to dst for flow
 * @emulate: emulate a single timeslot
 * @reset_sender: reset the state for a single sender
 * @stat: global emulation stats
 * @core_stats: stats per emulation core
 * @admitted: the current admitted struct
 * @packet_mempool: pool of packet structs
 * @endpoint_groups: representations of groups of endpoints
 * @routers: representations of routers
 * @q_new_packets: queue of packets from comm core to endpoint groups
 * @admitted_traffic_mempool: pool of admitted traffic structs
 * @q_admitted_out: queue of admitted structs to comm core
 */
class Emulation {
public:
	Emulation(struct fp_mempool *admitted_traffic_mempool,
		    struct fp_ring *q_admitted_out, struct fp_mempool *packet_mempool,
		    struct fp_ring **packet_queues, void *args);
	~Emulation();
	void add_backlog(uint16_t src, uint16_t dst, uint16_t flow, uint32_t amount);
	void emulate();
	void reset_sender(uint16_t src);
	struct emu_admission_statistics			stat;
	struct emu_admission_core_statistics	core_stats; /* 1 core for now */
	struct emu_admitted_traffic				*admitted;
	struct fp_mempool						*packet_mempool;
private:
	EndpointGroup							*endpoint_groups[EMU_NUM_ENDPOINT_GROUPS];
	Router									*routers[EMU_NUM_ROUTERS];
	struct fp_ring							*q_new_packets[EMU_NUM_ENDPOINT_GROUPS];
	struct fp_mempool						*admitted_traffic_mempool;
	struct fp_ring							*q_admitted_out;
};

#endif /* EMULATION_H_ */
