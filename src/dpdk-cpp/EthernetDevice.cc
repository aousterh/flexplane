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
	(*this).rx_split_hdr_size(0)
		.rx_header_split(0)
		.rx_hw_ip_checksum(1)
		.rx_hw_vlan_filter(0)
		.rx_jumbo_frame(0)
		.rx_hw_strip_crc(0);

}

dpdk::EthernetDeviceConfig::~EthernetDeviceConfig() {}
