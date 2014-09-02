/*
 * port.c
 *
 *  Created on: September 1, 2014
 *      Author: aousterh
 */

#include "port.h"
#include "api.h"
#include "packet.h"
#include "../graph-algo/fp_ring.h"


/*
 * Functions provided by the emulation framework for emulation algorithms to
 * call.
 */

void send_packet(struct emu_port *port, struct emu_packet *packet) {
	if (fp_ring_enqueue(port->q, packet) == -ENOBUFS) {
		// TODO: log failure
		drop_packet(packet);
	}
}

struct emu_packet *receive_packet(struct emu_port *port) {
	struct emu_packet *packet;

	if (fp_ring_dequeue(port->q, (void **) &packet) != 0)
		return NULL;

	return packet;
}
