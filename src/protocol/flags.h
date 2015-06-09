/*
 * flags.h
 *
 *  Created on: August 27, 2014
 *      Author: aousterh
 */

#ifndef FLAGS_H_
#define FLAGS_H_

#include "platform/generic.h"

/* This file contains information about encoding/decoding flags and extra data
 * in control packets used in emulation. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define FLAGS_MASK	0xF

/* flags for emulation */
#define EMU_FLAGS_NONE		0
#define EMU_FLAGS_DROP		1
#define EMU_FLAGS_ECN_MARK	2
#define EMU_FLAGS_MODIFY	3 /* modify headers in some other way */

/* parameters for areq data */
#define MAX_REQ_DATA_BYTES		8 /* must be a multiple of two */
#define AREQ_DATA_TYPE_NONE		0 /* no areq data used */
#define AREQ_DATA_TYPE_UNSPEC	1 /* unspecified data type, assumes MAX_REQ_DATA_BYTES */

/* parameters for alloc data */
#define MAX_ALLOC_DATA_BYTES	2 /* must be a multiple of two */
#define ALLOC_DATA_TYPE_NONE	0 /* only flags, no additional data */
#define ALLOC_DATA_TYPE_UNSPEC	1 /* unspecified data type, assumes MAX_ALLOC_DATA_BYTES */

#if defined(FASTPASS_CONTROLLER)
/**
 * Return the number of bytes per request data sent from endpoints to the
 * arbiter.
 */
static inline u16 emu_req_data_bytes(void) {
	u16 req_data_bytes;

#if (defined(DROP_TAIL) || defined(RED) || defined(DCTCP) || defined(HULL) \
	||	defined(ROUND_ROBIN) || defined(PRIO_QUEUEING))
	req_data_bytes = 0;
#else
	req_data_bytes = MAX_REQ_DATA_BYTES;
#endif

	return req_data_bytes;
}

/**
 * Return the number of additional bytes of data per alloc sent from the
 * arbiter back to an endpoint.
 */
static inline u16 emu_alloc_data_bytes(void) {
	u16 alloc_data_bytes;

#if (defined(DROP_TAIL) || defined(RED) || defined(DCTCP) || defined(HULL) \
	||	defined(ROUND_ROBIN) || defined(PRIO_QUEUEING))
	alloc_data_bytes = 0;
#else
	alloc_data_bytes = MAX_ALLOC_DATA_BYTES;
#endif

	return alloc_data_bytes;
}
#endif

/**
 * Return the type of areq data for this network scheme.
 */
static inline u8 areq_data_type_from_scheme(char *scheme) {
	if (strcmp(scheme, "drop_tail") == 0 || strcmp(scheme, "red") == 0 ||
			strcmp(scheme, "dctcp") == 0)
		return AREQ_DATA_TYPE_NONE;
	else
		return AREQ_DATA_TYPE_UNSPEC;
}

/**
 * Return the number of bytes per areq data for this network scheme.
 */
static inline u8 areq_data_bytes_from_scheme(char *scheme) {
	if (strcmp(scheme, "drop_tail") == 0 || strcmp(scheme, "red") == 0 ||
			strcmp(scheme, "dctcp") == 0)
		return 0;
	else
		return MAX_REQ_DATA_BYTES;
}

/**
 * Return the type of alloc data for this network scheme.
 */
static inline u8 alloc_data_type_from_scheme(char *scheme) {
	if (strcmp(scheme, "drop_tail") == 0 || strcmp(scheme, "red") == 0 ||
			strcmp(scheme, "dctcp") == 0)
		return ALLOC_DATA_TYPE_NONE;
	else
		return ALLOC_DATA_TYPE_UNSPEC;
}

/**
 * Return the number of bytes per alloc data for this network scheme.
 */
static inline u8 alloc_data_bytes_from_scheme(char *scheme) {
	if (strcmp(scheme, "drop_tail") == 0 || strcmp(scheme, "red") == 0 ||
			strcmp(scheme, "dctcp") == 0)
		return 0;
	else
		return MAX_ALLOC_DATA_BYTES;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FLAGS_H_ */
