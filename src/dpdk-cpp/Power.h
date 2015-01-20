/*
 * Power.h
 *
 *  Created on: Dec 30, 2014
 *      Author: yonch
 */

#ifndef POWER_H_
#define POWER_H_

namespace dpdk {

class Power {
public:
	Power(unsigned lcore_id);
	~Power();

	/** see: rte_power_get_freq */
	uint32_t get_freq();
	/** see: rte_power_freq_max */
	void freq_max();

private:
	unsigned m_lcore_id;
};

} /* namespace dpdk */

#endif /* POWER_H_ */
