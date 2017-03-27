/*
 * PyClassifier.h
 *
 *  Created on: Dec 19, 2014
 *      Author: yonch
 */

#ifndef CLASSIFIERS_PYCLASSIFIER_H_
#define CLASSIFIERS_PYCLASSIFIER_H_

#include <stdexcept>
#include "../composite.h"

/**
 * A classifier that implements classify as a virtual function, to enable
 *   run-time polymorphism.
 */
class PyClassifier : public Classifier {
public:
	PyClassifier() {};
	virtual ~PyClassifier() {};

	virtual uint32_t classify(struct emu_packet *pkt, uint32_t port)
		{
			throw std::runtime_error("not implemented");
		}

};

#endif /* CLASSIFIERS_PYCLASSIFIER_H_ */
