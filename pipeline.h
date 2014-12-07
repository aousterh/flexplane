/*
 * pipeline.h
 *
 *  Created on: Dec 7, 2014
 *      Author: yonch
 */

/**
 * Inspired by rte_pipeline
 */

#ifndef PIPELINE_H_
#define PIPELINE_H_

#define PIPELINE_MAX_PORTS 			8
#define PIPELINE_MAX_PROCESSORS 	8

/**
 * Port callback
 * Reads at most n_elems into elems[].
 * @param n_elems: maximum number of elements to read into elems
 * @return the number of RX elements, or negative value on error
 */
typedef int (*fp_port_rx)(void *port, void **elems, uint32_t n_elems);

/**
 * Processing element
 * Processes elements in @elems, only those with 1 bits in @mask
 * @param processor: the processor
 * @param elems: elements to process
 * @param mask: the mask of elements to process
 * @return a new mask of elements to be processed by following processors
 */
typedef uint64_t (*fp_processor)(void *processor, void **elems, uint64_t mask);

struct fp_pipeline {
	/* input ports */
	uint32_t 		n_ports;
	fp_port_rx 		port_func[PIPELINE_MAX_PORTS];
	void *			port[PIPELINE_MAX_PORTS];

	/* processing */
	uint32_t		n_processors;
	fp_processor	processor_callback[PIPELINE_MAX_PROCESSORS];
	void *			processor[PIPELINE_MAX_PROCESSORS];
};

/**
 * Runs one burst of packets from each port through all processors
 * @return: 0 on success, negative on failure
 */
int pipeline_run(struct fp_pipeline *p);


#endif /* PIPELINE_H_ */
