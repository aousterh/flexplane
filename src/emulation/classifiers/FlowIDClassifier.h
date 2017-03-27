/*
 * TorClassifier.h
 *
 *  Created on: Dec 17, 2014
 *      Author: yonch
 */

#ifndef CLASSIFIERS_FLOWIDCLASSIFIER_H_
#define CLASSIFIERS_FLOWIDCLASSIFIER_H_

#include <stdint.h>
#include "../composite.h"

/**
 * Classifies packets using the packet's flow ID.
 */
class FlowIDClassifier : public Classifier {
public:
	inline uint32_t classify(struct emu_packet *pkt, uint32_t port)
	{
		return pkt->flow;
	}
};

#endif /* CLASSIFIERS_FLOWIDCLASSIFIER_H_ */
