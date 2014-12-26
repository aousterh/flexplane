/*
 * SingleQueueClassifier.h
 *
 *  Created on: Dec 25, 2014
 *      Author: yonch
 */

#ifndef CLASSIFIERS_SINGLEQUEUECLASSIFIER_H_
#define CLASSIFIERS_SINGLEQUEUECLASSIFIER_H_

/**
 * A trivial classifier: classifies all packets into queue 0
 */
class SingleQueueClassifier : public Classifier {
public:
	inline uint32_t classify(struct emu_packet *pkt, uint32_t port)
	{
		return 0;
	}
};

#endif /* CLASSIFIERS_SINGLEQUEUECLASSIFIER_H_ */
