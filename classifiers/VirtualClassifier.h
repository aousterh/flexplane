/*
 * VirtualClassifier.h
 *
 *  Created on: Dec 19, 2014
 *      Author: yonch
 */

#ifndef CLASSIFIERS_VIRTUALCLASSIFIER_H_
#define CLASSIFIERS_VIRTUALCLASSIFIER_H_

#include <stdexcept>
#include "../composite.h"

/**
 * A classifier that implements classify as a virtual function, to enable
 *   run-time polymorphism.
 *
 * Envisioned for Python wrapper support
 */
class VirtualClassifier : public Classifier {
public:
	VirtualClassifier() {};
	virtual ~VirtualClassifier() {};

	virtual void classify(struct emu_packet *pkt, uint32_t *port,
			uint32_t *queue)
		{
		printf("pkt=%p port=%u queue=%u\n", pkt, *port, *queue);
		throw std::runtime_error("not implemented"); }
};

#endif /* CLASSIFIERS_VIRTUALCLASSIFIER_H_ */
