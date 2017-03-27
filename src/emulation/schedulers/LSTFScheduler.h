/*
 * LSTFScheduler.h
 *
 *  Created on: November 17, 2015
 *      Author: lut
 */

#ifndef SCHEDULERS_LSTFSCHEDULER_H_
#define SCHEDULERS_LSTFSCHEDULER_H_

#include "composite.h"
#include "lstf_queue_bank.h"

#include <stdexcept>

/**
 * Schedules packets by dequeueing the lowest slack packet from each port.
 */
class LSTFScheduler : public Scheduler{
public:
        LSTFScheduler(LSTFQueueBank *bank) : m_bank(bank){}

        inline struct emu_packet *schedule(uint32_t output_port, uint64_t cur_time,
                        Dropper *dropper)
        {
                if (unlikely(m_bank->empty(output_port)))
                        throw std::runtime_error("called schedule on an empty port");
                else 
                        return m_bank->dequeue_least_slack(output_port, cur_time);
        }

        inline uint64_t *non_empty_port_mask(){
                return m_bank->non_empty_port_mask();
        }

protected:
        LSTFQueueBank *m_bank;
};

#endif /* SCHEDULERS_LSTFSCHEDULER_H_ */
