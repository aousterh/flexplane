/*
 * SourceIDClassifier.h
 *
 *  Created on: Dec 17, 2014
 *      Author: yonch
 */

#ifndef CLASSIFIERS_SOURCEIDCLASSIFIER_H_
#define CLASSIFIERS_SOURCEIDCLASSIFIER_H_

#include <stdint.h>
#include "../composite.h"

/**
 * Classifies packets by source ID.
 *
 * source X would go into queue X (mod 64)
 */
class SourceIDClassifier : public Classifier {
public:
	SourceIDClassifier() {}

	inline uint32_t classify(struct emu_packet *pkt, uint32_t port)
	{
		return (pkt->src & 0x3F);
	}
};

#endif /* CLASSIFIERS_SOURCEIDCLASSIFIER_H_ */
