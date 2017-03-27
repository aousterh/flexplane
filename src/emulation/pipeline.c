/*
 * pipeline.c
 *
 *  Created on: Dec 7, 2014
 *      Author: yonch
 */

#include "pipeline.h"

/**
 * The number of elements to request from each port. Must be smaller than 64
 */
#define PIPELINE_BURST_SIZE		64

int pipeline_run(struct fp_pipeline *p)
{
	uint32_t i, j;
	void *elems[PIPELINE_BURST_SIZE];
	int n_rx;
	uint64_t mask;

	/* for each port */
	for (i = 0; i < p->n_ports; i++) {
		fp_port_rx port_func = p->port_func[i];
		void *port = p->port[i];

		n_rx = port_func(port, elems, PIPELINE_BURST_SIZE);
		if (n_rx < 0)
			return n_rx;

		mask = (uint64_t)(-1) >> (sizeof(uint64_t) * 8 - n_rx);

		/* for each processor */
		for (j = 0; j < p->n_processors; j++) {
			fp_processor proc_func = p->processor_callback[j];
			void *proc = p->processor[j];

			mask = proc_func(proc, elems, mask);
		}
	}

	return 0;
}
