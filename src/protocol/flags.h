/*
 * flags.h
 *
 *  Created on: August 27, 2014
 *      Author: aousterh
 */

#ifndef FLAGS_H_
#define FLAGS_H_

#include "platform/generic.h"

/* This file contains information about encoding/decoding flags in Fastpass
 * control packets. */

#define FLAGS_MASK	0xF

/* flags for emulation */
#define EMU_FLAGS_NONE		0
#define EMU_FLAGS_DROP		1
#define EMU_FLAGS_ECN_MARK	2

/* parameters for areq data */
#define MAX_REQ_DATA_BYTES		8 /* must be a multiple of two */

/*
 * Return the number of bytes per request data sent from endpoints to the
 * arbiter.
 */
static inline u16 emu_req_data_bytes(void) {
	u16 req_data_bytes;

#if (defined(DROP_TAIL) || defined(RED) || defined(DCTCP))
	req_data_bytes = 0;
#else
	req_data_bytes = MAX_REQ_DATA_BYTES;
#endif

	return req_data_bytes;
}

#endif /* FLAGS_H_ */
