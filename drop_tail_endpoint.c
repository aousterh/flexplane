/*
 * drop_tail_endpoint.c
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#include "drop_tail_endpoint.h"

#include "../graph-algo/admissible_algo_log.h"

/**
 * Initialize an endpoint.
 * @return 0 on success, negative value on error.
 */
int drop_tail_endpoint_init(struct emu_endpoint *ep) {
	return 0;
};

/**
 * Reset an endpoint. This happens when endpoints lose sync with the
 * arbiter. To resync, a reset occurs, then backlogs are re-added based
 * on endpoint reports.
 */
void drop_tail_endpoint_reset(struct emu_endpoint *ep) {};

/**
 * Cleanup state and memory. Called when emulation terminates.
 */
void drop_tail_endpoint_cleanup(struct emu_endpoint *ep) {};

/**
 * Emulate one timeslot at a given endpoint.
 */
void drop_tail_endpoint_emulate(struct emu_endpoint *ep) {
	struct emu_packet *packet;
	struct emu_port *port;

	/* try to dequeue one packet - return if there are none */
	packet = dequeue_packet_at_endpoint(ep);
	if (packet == NULL)
		return;

	/* try to transmit the packet to the next router */
	port = endpoint_port(ep);
	send_packet(port, packet);
}
