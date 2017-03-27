/*
 * PyQueueManager.h
 *
 *  Created on: Dec 19, 2014
 *      Author: yonch
 */

#ifndef QUEUE_MANAGERS_PYQUEUEMANAGER_H_
#define QUEUE_MANAGERS_PYQUEUEMANAGER_H_

#include "../composite.h"

class PyQueueManager: public QueueManager {
public:
	PyQueueManager() {};
	virtual ~PyQueueManager() {};

	virtual void enqueue(struct emu_packet *pkt, uint32_t port, uint32_t queue,
			uint64_t cur_time) {}
};

#endif /* QUEUE_MANAGERS_PYQUEUEMANAGER_H_ */
