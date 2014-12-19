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
 *
 * Envisioned for Python wrapper support
 */
class PyClassifier : public Classifier {
public:
	PyClassifier() {};
	virtual ~PyClassifier() {};

	virtual void py_classify(struct emu_packet *pkt, uint32_t *port,
			uint32_t *queue)
		{
			throw std::runtime_error("not implemented");
		}

	void classify(struct emu_packet *pkt, uint32_t *port,
			uint32_t *queue)
		{
			py_classify(pkt,port,queue);
		}


};

#endif /* CLASSIFIERS_PYCLASSIFIER_H_ */
