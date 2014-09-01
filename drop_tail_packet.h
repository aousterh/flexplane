/*
 * drop_tail_packet.h
 *
 *  Created on: September 1, 2014
 *      Author: aousterh
 */

#ifndef DROP_TAIL_PACKET_H_
#define DROP_TAIL_PACKET_H_

#include "api.h"

static struct packet_ops drop_tail_packet_ops = {
		.priv_size	= 0, /* no private state necessary */
};

#endif /* DROP_TAIL_PACKET_H_ */
