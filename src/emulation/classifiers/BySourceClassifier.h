/*
 * BySourceClassifier.h
 *
 *  Created on: Dec 17, 2014
 *      Author: yonch
 */

#ifndef CLASSIFIERS_BYSOURCECLASSIFIER_H_
#define CLASSIFIERS_BYSOURCECLASSIFIER_H_

#include <stdint.h>
#include "../composite.h"

struct prio_by_src_args {
    uint16_t q_capacity;
    uint32_t n_hi_prio;
    uint32_t n_med_prio;
};

/**
 * Classifies packets by source.
 *
 * Some sources are hi priority (queue 0), some medium (queue 1) and the rest
 *    are low priority (queue 2).
 */
class BySourceClassifier : public Classifier {
public:
	BySourceClassifier(uint32_t n_hi_prio,
					   uint32_t n_med_prio)
	  : hi_prio_thresh(n_hi_prio),
		med_prio_thresh(n_hi_prio + n_med_prio)
	{}

	inline uint32_t classify(struct emu_packet *pkt, uint32_t port)
	{
		if (pkt->src < hi_prio_thresh)
			return 0;
		else if (pkt->src < med_prio_thresh)
			return 1;

		return 2; /* low prio */
	}

private:
	uint32_t hi_prio_thresh;
	uint32_t med_prio_thresh;
};

#endif /* CLASSIFIERS_BYSOURCECLASSIFIER_H_ */
