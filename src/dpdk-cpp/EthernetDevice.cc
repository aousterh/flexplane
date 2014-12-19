/*
 * EthernetDevice.cpp
 *
 *  Created on: Dec 18, 2014
 *      Author: yonch
 */

#include "EthernetDevice.h"

namespace dpdk {

EthernetDevice::EthernetDevice() {
	// TODO Auto-generated constructor stub

}

EthernetDevice::~EthernetDevice() {
	// TODO Auto-generated destructor stub
}

} /* namespace dpdk */

dpdk::EthernetDeviceConfig::EthernetDeviceConfig()
{
	memset(&m_conf, 0, sizeof(struct rte_eth_conf));

	/* try to put sensible defaults */


}

dpdk::EthernetDeviceConfig::~EthernetDeviceConfig() {}
