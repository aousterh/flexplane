/*
 * drop_tail_endpoint.h
 *
 *  Created on: August 30, 2014
 *      Author: aousterh
 */

#ifndef DROP_TAIL_ENDPOINT_H_
#define DROP_TAIL_ENDPOINT_H_

#include "api.h"

/**
 * Initialize an endpoint.
 * @return 0 on success, negative value on error.
 */
int drop_tail_endpoint_init(struct emu_endpoint *ep);

/**
 * Reset an endpoint. This happens when endpoints lose sync with the
 * arbiter. To resync, a reset occurs, then backlogs are re-added based
 * on endpoint reports.
 */
void drop_tail_endpoint_reset(struct emu_endpoint *ep);

/**
 * Cleanup state and memory. Called when emulation terminates.
 */
void drop_tail_endpoint_cleanup(struct emu_endpoint *ep);

/**
 * Emulate one timeslot at a given endpoint.
 */
void drop_tail_endpoint_emulate(struct emu_endpoint *endpoint);

#endif /* DROP_TAIL_ENDPOINT_H_ */
